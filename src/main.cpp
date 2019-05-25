//
// Created by Evgeniy Morozov on 2019-05-24.
//
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>
#include <set>
#include <fstream>
#include <functional>
#include <numeric>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <fftw3.h>

using namespace boost::program_options;

int main(int argc, const char * const argv[])
{
    std::int32_t start, end;
    std::string out_dir;
    std::vector<std::string> filenames;

    try {
        options_description available_options("Available Options");
        available_options.add_options()
                ("help,h", "Show help")
                ("start,s", value<std::int32_t>(&start)->default_value(0), "Analysis start period")
                ("out_directory,o", value<std::string>(&out_dir)->default_value(""), "Output directory")
                ("end,e", value<std::int32_t>(&end)->default_value(-1), "Analysis end period");

        options_description hidden_opts;
        hidden_opts.add_options()
                ("file_list", value<std::vector<std::string>>(&filenames)->multitoken(), "Files to process");

        options_description all_opts;
        all_opts.add(available_options);
        all_opts.add(hidden_opts);

        positional_options_description pos_desc;
        pos_desc.add("file_list", -1);

        command_line_parser parser{argc, argv};
        parser.options(all_opts).positional(pos_desc).allow_unregistered();
        parsed_options parsed_options = parser.run();

        variables_map vm;
        store(parsed_options, vm);
        notify(vm);

        if (vm.count("help")) {
            std::cout << available_options << std::endl;
        } else if (!vm.count("file_list")) {
            throw error("Nothing to do - no file specified");
        }
    }
    catch (const error &ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::set<boost::filesystem::path> filename_set;

    for (auto const & filename : filenames) {
        boost::filesystem::path target_path(filename);
        auto target_path_name = target_path.parent_path().string();
        if (target_path_name.empty()) {
            target_path_name = "./";
        }

        // Convert glob pattern to regex escaping all possible control sequences
        auto filename_wildcarded = target_path.filename().string();
        filename_wildcarded = std::regex_replace(filename_wildcarded, std::regex(R"((\~|\^|\{|\}|\s|\(|\)|\/|\\))"), "\\$1");
        filename_wildcarded = std::regex_replace(filename_wildcarded, std::regex("\\."), "\\.");
        filename_wildcarded = std::regex_replace(filename_wildcarded, std::regex("\\*"), ".*");
        filename_wildcarded = std::regex_replace(filename_wildcarded, std::regex("\\?"), ".");

        const std::regex my_filter(filename_wildcarded);

        boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
        for( boost::filesystem::directory_iterator i(target_path_name); i != end_itr; ++i ) {
            if(!boost::filesystem::is_regular_file(i->status())) {
                continue;
            }

            auto current_filename = i->path().filename().string();
            if (!std::regex_match(current_filename, my_filter)) {
                continue;
            }

            filename_set.insert(i->path());
        }
    }

    for (auto const& path : filename_set) {
        std::vector<double> data;
        std::ifstream f(path.string());
        std::istream_iterator<double> input(f);
        std::copy(input, std::istream_iterator<double>(), std::back_inserter(data));
        if (data.empty()) {
            std::cerr << "File " << path << " does not contain any data, skipping" << std::endl;
            continue;
        }
        //std::copy(data.begin(), data.end(), std::ostream_iterator<double>(std::cout, "\n"));
        if (end == -1) {
            end = data.size();
        }

        // Sanity check for analysis window
        if (start >= data.size() || end > data.size()) {
            std::cerr << "File " << path << " contains less data than expected, skipping" << std::endl;
            continue;
        }
        auto span = end - start;
        if (span < 0 || span > data.size()) {
            std::cerr << "Incorrect settings for data start and end" << std::endl;
        }

        // std::cout << "FFT " << path.string() << ":" << std::endl;

        auto fft_out = fftw_alloc_complex(data.size());

        // TODO: This is not entirely correct. As the FFTW spec says, plan creation
        // could possibly erase data in the input array, so plans should be created
        // at first. But when used with FFTW_ESTIMATE FFTW does not touch the input
        // array but FFTW team doesn't give any promises on that.
        auto fftw_plan = fftw_plan_dft_r2c_1d((int)data.size(), data.data(), fft_out, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
        fftw_execute(fftw_plan);

        for (auto i = 1; i < data.size() / 2 + 1; ++i) {
            fft_out[data.size() - i][0] = fft_out[i][0];
            fft_out[data.size() - i][1] = -fft_out[i][1];
        }

        fftw_destroy_plan(fftw_plan);

        std::vector<double> result(span, 0);
        fftw_plan = fftw_plan_dft_c2r_1d(span, fft_out + start, result.data(), FFTW_ESTIMATE);
        fftw_execute(fftw_plan);
        fftw_free(fft_out);
        fftw_destroy_plan(fftw_plan);

        // Normalize the data
        std::transform(result.begin(), result.end(), result.begin(),
                       std::bind(std::multiplies<double>(), std::placeholders::_1, 1.0 / data.size()));

        // std::copy(result.begin(), result.end(), std::ostream_iterator<double>(std::cout, "\n"));

        auto min = std::min_element(result.begin(), result.end());
        auto max = std::max_element(result.begin(), result.end());

        // std::cout << *min << " " << *max << std::endl;

        // Find local maximums (peaks in signal)
        std::vector<double> d_dx(result.size(), 0);
        std::adjacent_difference(result.begin(), result.end(), d_dx.begin());

        // Use set structure to simplify corner-cases handling such as there are two
        // elements in array and it
        std::set<int> peaks;
        if (d_dx.size() == 1) {
            peaks.insert(0);
        }

        bool high_peak_detected = false;
        for (auto i = 1; i < d_dx.size(); ++i) {
            if (d_dx[i - 1] > 0 && d_dx[i] <= 0) {
                high_peak_detected = true;
                peaks.insert(i - 1);

                continue;
            }

            if (high_peak_detected && d_dx[i] == 0) {
                peaks.insert(i - 1);

                continue;
            }

            high_peak_detected = false;
        }

        if ((high_peak_detected && *d_dx.end() == 0) || (d_dx.size() > 1 && *d_dx.end() > 0)) {
            peaks.insert(d_dx.size() - 1);
        }

        std::vector<int> peak_distances(peaks.size(), 0);
        std::adjacent_difference(peaks.begin(), peaks.end(), peak_distances.begin());

        // std::cout << " OK" << std::endl;
    }

    return 0;
}