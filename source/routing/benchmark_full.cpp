/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "raptor_api.h"
#include "raptor.h"
#include "georef/street_network.h"
#include "type/data.h"
#include "utils/timer.h"
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <random>
#include <fstream>
#include "utils/init.h"
#include "utils/csv.h"
#include <boost/algorithm/string/predicate.hpp>
#ifdef __BENCH_WITH_CALGRIND__
#include "valgrind/callgrind.h"
#endif

using namespace navitia;
using namespace routing;
namespace po = boost::program_options;
namespace ba = boost::algorithm;

struct PathDemand {
    type::idx_t start;
    type::idx_t target;
    unsigned int date;
    unsigned int hour;
    type::Mode_e start_mode = type::Mode_e::Walking;
    type::Mode_e target_mode = type::Mode_e::Walking;
};

struct Result {
    int duration;
    int time;
    int arrival;
    int nb_changes;

    Result(pbnavitia::Journey journey) : duration(journey.duration()), time(-1), arrival(journey.arrival_date_time()), nb_changes(journey.nb_transfers()) {}
};

int main(int argc, char** argv){
    navitia::init_app();
    po::options_description desc("Options de l'outil de benchmark");
    std::string file, output, stop_input_file;
    int iterations, start, target, date, hour;

    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    logger.setLogLevel(log4cplus::WARN_LOG_LEVEL);

    desc.add_options()
            ("help", "Show this message")
            ("interations,i", po::value<int>(&iterations)->default_value(100),
                     "Number of iterations (10 calcuations par iteration)")
            ("file,f", po::value<std::string>(&file)->default_value("data.nav.lz4"),
                     "Path to data.nav.lz4")
            ("start,s", po::value<int>(&start)->default_value(-1),
                    "Start of a particular journey")
            ("target,t", po::value<int>(&target)->default_value(-1),
                    "Target of a particular journey")
            ("date,d", po::value<int>(&date)->default_value(-1),
                    "Begginning date of a particular journey")
            ("hour,h", po::value<int>(&hour)->default_value(-1),
                    "Begginning hour of a particular journey")
            ("verbose,v", "Verbose debugging output")
            ("stop_files", po::value<std::string>(&stop_input_file), "File with list of start and target")
            ("output,o", po::value<std::string>(&output)->default_value("benchmark.csv"),
                     "Output file");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    bool verbose = vm.count("verbose");

    if (vm.count("help")) {
        std::cout << "This is used to benchmark journey computation" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    type::Data data;
    {
        Timer t("Chargement des données : " + file);
        data.load(file);
    }
    std::vector<PathDemand> demands;

    if (! stop_input_file.empty()) {
        //csv file should be the same as the output one
        CsvReader csv(stop_input_file, ',');
        csv.next();
        size_t cpt_not_found = 0;
        for (auto it = csv.next(); ! csv.eof(); it = csv.next()) {
            const auto it_start = data.pt_data->stop_areas_map.find(it[0]);
            if (it_start == data.pt_data->stop_areas_map.end()) {
                std::cout << "impossible to find " << it[0] << std::endl;
                cpt_not_found++;
                continue;
            }
            const auto start = it_start->second;

            const auto it_end = data.pt_data->stop_areas_map.find(it[2]);
            if (it_end == data.pt_data->stop_areas_map.end()) {
                std::cout << "impossible to find " << it[2] << std::endl;
                cpt_not_found++;
                continue;
            }
            const auto end = it_end->second;

            PathDemand demand;
            demand.start = start->idx;
            demand.target = end->idx;
            demand.hour = boost::lexical_cast<unsigned int>(it[5]);
            demand.date = boost::lexical_cast<unsigned int>(it[4]);
            demands.push_back(demand);
        }
        std::cout << "nb start not found " << cpt_not_found << std::endl;
    }
    else if(start != -1 && target != -1 && date != -1 && hour != -1) {
        PathDemand demand;
        demand.start = start;
        demand.target = target;
        demand.hour = hour;
        demand.date = date;
        demands.push_back(demand);
    } else {
        // Génération des instances
        std::random_device rd;
        std::mt19937 rng(31442);
        std::uniform_int_distribution<> gen(0,data.pt_data->stop_areas.size()-1);
        std::vector<unsigned int> hours{0, 28800, 36000, 72000, 86000};
        std::vector<unsigned int> days({date != 1 ? unsigned(date) : 7});
        if(data.pt_data->validity_patterns.front()->beginning_date.day_of_week().as_number() == 6)
            days.push_back(days.front() + 1);
        else
            days.push_back(days.front() + 6);

        for(int i = 0; i < iterations; ++i) {
            PathDemand demand;
            demand.start = gen(rng);
            demand.target = gen(rng);
            while(demand.start == demand.target
                    || ba::starts_with(data.pt_data->stop_areas[demand.start]->uri, "stop_area:SNC:")
                    || ba::starts_with(data.pt_data->stop_areas[demand.target]->uri, "stop_area:SNC:")) {
                demand.target = gen(rng);
                demand.start = gen(rng);
            }
            for(auto day : days) {
                for(auto hour : hours) {
                    demand.date = day;
                    demand.hour = hour;
                    demands.push_back(demand);
                }
            }
        }
    }

    // Calculs des itinéraires
    std::vector<Result> results;
    data.build_raptor();
    RAPTOR router(data);
    auto georef_worker = georef::StreetNetwork(*data.geo_ref);

    std::cout << "On lance le benchmark de l'algo " << std::endl;
    boost::progress_display show_progress(demands.size());
    Timer t("Calcul avec l'algorithme ");
    //ProfilerStart("bench.prof");
    int nb_reponses = 0;
#ifdef __BENCH_WITH_CALGRIND__
    CALLGRIND_START_INSTRUMENTATION;
#endif
    for(auto demand : demands){
        ++show_progress;
        Timer t2;
        auto date = data.pt_data->validity_patterns.front()->beginning_date + boost::gregorian::days(demand.date + 1) - boost::gregorian::date(1970, 1, 1);
        if (verbose){
            std::cout << data.pt_data->stop_areas[demand.start]->label
                      << ", " << demand.start
                      << ", " << data.pt_data->stop_areas[demand.target]->label
                      << ", " << demand.target
                      << ", " << date
                      << ", " << demand.hour
                      << "\n";
        }

        type::EntryPoint origin = type::EntryPoint(type::Type_e::StopArea, data.pt_data->stop_areas[demand.start]->uri, 0);
        type::EntryPoint destination = type::EntryPoint(type::Type_e::StopArea, data.pt_data->stop_areas[demand.target]->uri, 0);
        origin.coordinates = data.pt_data->stop_areas[demand.start]->coord;
        destination.coordinates = data.pt_data->stop_areas[demand.target]->coord;
        origin.streetnetwork_params.mode = demand.start_mode;
        origin.streetnetwork_params.offset = data.geo_ref->offsets[demand.start_mode];
        origin.streetnetwork_params.max_duration = navitia::seconds(15*60);
        origin.streetnetwork_params.speed_factor = 1;
        destination.streetnetwork_params.mode = demand.target_mode;
        destination.streetnetwork_params.offset = data.geo_ref->offsets[demand.target_mode];
        destination.streetnetwork_params.max_duration = navitia::seconds(15*60);
        destination.streetnetwork_params.speed_factor = 1;
        type::AccessibiliteParams accessibilite_params;
        auto resp = make_response(router, origin, destination,
              {DateTimeUtils::set(date.days(), demand.hour)}, true,
              accessibilite_params,
              {},
              georef_worker,
              false,
              true,
              std::numeric_limits<int>::max(), 10, false);

        if(resp.journeys_size() > 0) {
            ++ nb_reponses;

            Result result(resp.journeys(0));
            result.time = t2.ms();
            results.push_back(result);
        }
    }
    //ProfilerStop();
#ifdef __BENCH_WITH_CALGRIND__
    CALLGRIND_STOP_INSTRUMENTATION;
#endif

    std::cout << "Number of requests: " << demands.size() << std::endl;
    std::cout << "Number of results with solution: " << nb_reponses << std::endl;
}
