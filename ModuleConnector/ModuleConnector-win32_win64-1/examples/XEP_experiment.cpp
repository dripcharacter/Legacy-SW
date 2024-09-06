#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <direct.h>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "Data.hpp"
#include "ModuleConnector.hpp"
#include "XEP.hpp"
#include "DataRecorder.hpp"
#include "iostream"
#include "xtid.h"
#include "unistd.h"
#include "stdio.h"
#include "signal.h"
#include "X2M200.hpp"
#include "X4M300.hpp"
#include "functional"

/** \example XEP_experiment.cpp
 */

using namespace XeThru;

int handle_error(std::string message)
{
    std::cerr << "ERROR: " << message << std::endl;
    return 1;
}

void xep_app_init(XEP* xep)
{
    xep->x4driver_init();

    //Setting default values for XEP
    int dac_min = 949;
    int dac_max = 1100;
    int iteration = 64;
    int pps = 60;
    int fps = 25;
    float offset = 0.18;
    float fa1 = -0.4372839331626892;
    float fa2 = 9.18205738067627;
    int dc = 1;

    //Writing to module XEP
    xep->x4driver_set_dac_min(dac_min);
    xep->x4driver_set_dac_max(dac_max);
    xep->x4driver_set_iterations(iteration);
    xep->x4driver_set_pulses_per_step(pps);
    xep->x4driver_set_fps(fps);
    xep->x4driver_set_frame_area_offset(offset);
    xep->x4driver_set_frame_area(fa1, fa2);
    xep->x4driver_set_downconversion(dc);
}

std::vector<std::vector<float>> i_channel_data;
std::vector<std::vector<float>> q_channel_data;
std::vector<std::vector<float>> amplitude_data;
std::vector<std::vector<float>> phase_data;

volatile sig_atomic_t stop_recording;
void handle_sigint(int num)
{
    stop_recording = 1;
    if (i_channel_data.size()!=0)
    {
        std::cout<< "start making csv"<<std::endl;

        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        // yy_mm_dd_hh_mm_ss 형식으로 시간 문자열 만들기
        std::tm* tm_ptr = std::localtime(&now_time);
        std::stringstream ss;
        ss << std::put_time(tm_ptr, "%y_%m_%d_%H_%M_%S");
        std::string dir_name = ss.str();

        // 디렉터리 생성 (Windows)
        if (_mkdir(dir_name.c_str()) == 0) {
            std::cout << "mkdir success: " << dir_name << std::endl;
        } else {
            std::cerr << "mkdir fail" << std::endl;
        }
        
        // 파일 저장 경로
        std::string i_channel_file = dir_name+"/i_channel_data.csv";
        std::string q_channel_file = dir_name+"/q_channel_data.csv";
        std::string amplitude_file = dir_name+"/amplitude_data.csv";
        std::string phase_file = dir_name+"/phase_data.csv";

        // 각각의 데이터 벡터를 CSV 파일로 저장
        auto save_to_csv = [](const std::string &filename, const std::vector<std::vector<float>> &data) {
            std::ofstream file(filename);
            if (file.is_open()) {
                file << std::fixed << std::setprecision(15);
                for (const auto &row : data) {
                    for (size_t i = 0; i < row.size(); ++i) {
                        file << row[i];
                        if (i < row.size() - 1)
                            file << ",";
                    }
                    file << "\n";
                }
                file.close();
            } else {
                std::cerr << "ERROR: Could not open file " << filename << " for writing." << std::endl;
            }
        };

        // 각 데이터 저장
        save_to_csv(i_channel_file, i_channel_data);
        save_to_csv(q_channel_file, q_channel_data);
        save_to_csv(amplitude_file, amplitude_data);
        save_to_csv(phase_file, phase_data);

        std::cout << "==========================================================================================" << std::endl;
        std::cout << "i_data len: " << i_channel_data.size() << std::endl;
        std::cout << "q_data len: " << q_channel_data.size() << std::endl;
        std::cout << "amplitude_data len: " << amplitude_data.size() << std::endl;
        std::cout << "phase_data len: " << phase_data.size() << std::endl;
        std::cout << "==========================================================================================" << std::endl;

        std::cout << "Data saved to CSV files." << std::endl;

        exit(0);
    }
}

static void on_file_available(XeThru::DataType type, const std::string &filename)
{
    std::cout << "recorded file available for data type: "
              << DataRecorder::data_type_to_string(type) << std::endl;
    std::cout << "file: " << filename << std::endl;
}

static void on_meta_file_available(const std::string &session_id, const std::string &meta_filename)
{
    std::cout << "meta file available for recording with id: " << session_id << std::endl;
    std::cout << "file: " << meta_filename << std::endl;
}

int record(const std::string &device_name)
{
    using namespace XeThru;

    stop_recording = 0;
    signal(SIGINT, handle_sigint);

    ModuleConnector mc(device_name, 3);

    std::string FWID;
    XEP & xep = mc.get_xep();
    //TODO: ping하는 함수 추가하기
    xep_app_init(&xep);
    xep.get_system_info(0x02, &FWID);

    if (FWID == "XEP") {
        //Do nothing
    } else if (FWID == "xtapplication_m4") {
        // If Module X2M200, record baseband
        std::cout << "Program not supported for X2M200." << '\n';
    } else {
        std::string Module;
        X4M300 & x4m300=mc.get_x4m300();
        x4m300.set_sensor_mode(XTID_SM_STOP,0);
        x4m300.set_sensor_mode(XTID_SM_MANUAL,0);
        xep.get_system_info(0x01, &Module);
        std::cout << "Start XEP recording for " << Module << '\n';
    }

    DataRecorder &recorder = mc.get_data_recorder();

    const DataTypes data_types = AllDataTypes;
    const std::string output_directory = ".";

    if (recorder.start_recording(data_types, output_directory) != 0) {
        std::cout << "Failed to start recording" << std::endl;
        return 1;
    }

    {
        DataRecorder::FileAvailableCallback callback = std::bind(&on_file_available,
                                                                 std::placeholders::_1,
                                                                 std::placeholders::_2);
        recorder.subscribe_to_file_available(AllDataTypes, callback);
    }

    {
        DataRecorder::MetaFileAvailableCallback callback = std::bind(&on_meta_file_available,
                                                                     std::placeholders::_1,
                                                                     std::placeholders::_2);
        recorder.subscribe_to_meta_file_available(callback);
    }

    // start streaming data
    xep.x4driver_set_fps(5);
    while (!stop_recording) {
        usleep(1000);
    }

    return 0;
}

int monitoring(const std::string &device_name)
{
    signal(SIGINT, handle_sigint);
    const unsigned int log_level = 0;
    ModuleConnector mc(device_name, log_level);
    XEP & xep = mc.get_xep();

    std::string FWID;
    //If the module is a X4M200 or X4M300 it needs to be put in manual mode
    xep.get_system_info(0x02, &FWID);
    if (FWID != "XEP") {
        //Module X4M300 or X4M200
        std::string Module;
        X4M300 & x4m300 = mc.get_x4m300();
        x4m300.set_sensor_mode(XTID_SM_STOP,0);
        x4m300.set_sensor_mode(XTID_SM_MANUAL,0);
        xep.get_system_info(0x01, &Module);
        std::cout << "Module " << Module << " set to XEP mode"<< std::endl;
    }

    // Configure XEP
    xep_app_init(&xep);
    // DataFloat
    XeThru::DataFloat test;
    // Check packets queue
    int packets = xep.peek_message_data_float();
    std::cout << "Packets in queue: " << packets<<'\n';

    

    // Printing output until KeyboardInterrupt
    // This data is not readable.
    std::cout << "Printing out data: " << '\n';
    while (true) {
        if (xep.read_message_data_float(&test)) {
            return handle_error("read_message_data_float failed");
        }
        std::vector<float> data_vec = test.get_data();
        std::vector<float> i_channel, q_channel, amplitude, phase;

        for (size_t i = 0; i < data_vec.size() / 2; i++) {
            i_channel.push_back(data_vec[i]);
            q_channel.push_back(data_vec[i + data_vec.size() / 2]);
            amplitude.push_back(sqrt(pow(data_vec[i + data_vec.size() / 2], 2) + pow(data_vec[i], 2)));
            phase.push_back(atan2(data_vec[i + data_vec.size() / 2], data_vec[i]));
        }

        i_channel_data.push_back(i_channel);
        q_channel_data.push_back(q_channel);
        amplitude_data.push_back(amplitude);
        phase_data.push_back(phase);
        printf("this is the size: %lld\n", data_vec.size());
        // std::cout << "vector size is " << amplitude_data.size() << " * " << amplitude_data[0].size() << std::endl;
        // std::cout << "i_channel_data vector" << std::endl;
        // for (size_t i=0; i<i_channel_data[i_channel_data.size()-1].size(); i++)
        //     std::cout << i_channel_data[i_channel_data.size()-1][i] << " ";
        // std::cout << std::endl;
        // std::cout << "q_channel_data vector" << std::endl;
        // for (size_t i=0; i<q_channel_data[q_channel_data.size()-1].size(); i++)
        //     std::cout << q_channel_data[q_channel_data.size()-1][i] << " ";
        // std::cout << std::endl;
        // std::cout << "amplitude vector" << std::endl;
        // for (size_t i=0; i<amplitude_data[amplitude_data.size()-1].size(); i++)
        //     std::cout << amplitude_data[amplitude_data.size()-1][i] << " ";
        // std::cout << std::endl;
        // std::cout << "phase vector" << std::endl;
        // for (size_t i=0; i<phase_data[phase_data.size()-1].size(); i++)
        //     std::cout << phase_data[phase_data.size()-1][i] << " ";
        // std::cout << std::endl;
        std::cout << "==========================================================================================" << std::endl;
        std::cout << "i_data len: " << i_channel_data.size() << std::endl;
        std::cout << "q_data len: " << q_channel_data.size() << std::endl;
        std::cout << "amplitude_data len: " << amplitude_data.size() << std::endl;
        std::cout << "phase_data len: " << phase_data.size() << std::endl;
        std::cout << "==========================================================================================" << std::endl;
    }
    return 0;
}

int main(int argc, char ** argv)
{
    if (argc < 2) {
        std::cout << "usage: XEP_experiment <com port or device file>" << std::endl;
        return 1;
    }
    monitoring(argv[1]);

    return 0;
}