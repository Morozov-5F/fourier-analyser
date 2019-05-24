//
// Created by Evgeniy Morozov on 2019-05-24.
//
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

#include <boost/program_options.hpp>

using namespace boost::program_options;

int main(int argc, const char * const argv[])
{
    std::int64_t start, end;
    std::string out_dir;

    try {
        options_description available_options("Available Options");
        available_options.add_options()
                ("help,h", "Show help")
                ("start,s", value<std::int64_t>(&start)->default_value(0), "Analysis start period")
                ("out_directory,o", value<std::string>(&out_dir)->default_value(""), "Output directory")
                ("end,e", value<std::int64_t>(&end)->default_value(-1), "Analysis end period");

        options_description hidden_opts;
        hidden_opts.add_options()
                ("file_list", value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(), "Files to process");

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
        } else if (vm.count("file_list")) {
            std::copy(vm["file_list"].as<std::vector<std::string>>().begin(),
                      vm["file_list"].as<std::vector<std::string>>().end(),
                      std::ostream_iterator<std::string>{std::cout, "\n"});
        }
    }
    catch (const error &ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}