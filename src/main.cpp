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

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <fftw3.h>

using namespace boost::program_options;

int main(int argc, const char * const argv[])
{
    std::int64_t start, end;
    std::string out_dir;
    std::vector<std::string> filenames;

    try {
        options_description available_options("Available Options");
        available_options.add_options()
                ("help,h", "Show help")
                ("start,s", value<std::int64_t>(&start)->default_value(0), "Analysis start period")
                ("out_directory,o", value<std::string>(&out_dir)->default_value(""), "Output directory")
                ("end,e", value<std::int64_t>(&end)->default_value(-1), "Analysis end period");

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
            std::cout << "File " << path << " does not contain any data, skipping" << std::endl;
            continue;
        }
        //std::copy(data.begin(), data.end(), std::ostream_iterator<double>(std::cout, "\n"));

        auto fft_out = fftw_alloc_complex(data.size());

        auto fftw_plan = fftw_plan_dft_r2c_1d((int)data.size(), data.data(), fft_out, FFTW_ESTIMATE);
        fftw_execute(fftw_plan);

        for (auto i = 1; i < data.size() / 2 + 1; ++i) {
            fft_out[data.size() - i][0] = fft_out[i][0];
            fft_out[data.size() - i][1] = -fft_out[i][1];
        }

        for (auto i = 0; i < data.size(); ++i) {
            std::cout << data[i] << ": " << fft_out[i][0] << " " << fft_out[i][1] << std::endl;
        }

        fftw_destroy_plan(fftw_plan);
        fftw_free(fft_out);
    }

    return 0;
}