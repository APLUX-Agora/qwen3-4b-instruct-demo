// SPDX-License-Identifier: Apache-2.0
#include <aidlux/aidgen/aidgen.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace aplux::aidgen;

static void check(ErrorCode code, const char* action) {
    if (code != ErrorCode::SUCCESS)
        throw std::runtime_error(std::string(action) + " failed (" + std::to_string(static_cast<int>(code)) + ")");
}

int main(int argc, char** argv) {
    std::string models, prompt;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--models" || arg == "--prompt") && i + 1 < argc) {
            (arg == "--models" ? models : prompt) = argv[++i];
        } else {
            std::cerr << "Usage: qwen3_demo --models <target-directory> --prompt <text>\n";
            return 2;
        }
    }
    if (models.empty() || prompt.empty()) {
        std::cerr << "Both --models and --prompt are required.\n";
        return 2;
    }
    try {
        // Model configuration paths are relative to this directory.
        std::filesystem::current_path(std::filesystem::absolute(models));
        if (!std::filesystem::is_regular_file("config.json"))
            throw std::runtime_error("Missing config.json; download the complete target directory first");
        set_log_level(LogLevel::ERROR);
        ContextProperties properties;
        properties.stream = true;
        properties.enable_profiler = true;
        properties.enable_prompt_cache = false;
        auto context = Context::create_instance("config.json", properties);
        if (!context) throw std::runtime_error("Could not create AidGen context");
        check(context->initialize(), "Context initialization");
        auto generator = Generator::create_instance(context);
        if (!generator) throw std::runtime_error("Could not create AidGen generator");
        struct Finalize {
            Generator* value;
            ~Finalize() { if (value) value->finalize(); }
        } finalize{generator.get()};
        check(generator->initialize(), "Generator initialization");
        const std::string formatted =
            "<|im_start|>system\nYou are Qwen, created by Alibaba Cloud. You are a helpful assistant.<|im_end|>\n"
            "<|im_start|>user\n" + prompt + "<|im_end|>\n<|im_start|>assistant\n";
        bool ended = false, failed = false;
        std::string response;
        std::cout << "Assistant: " << std::flush;
        check(generator->run(formatted, [&](const GenEvent& event, void*) {
            if (event.state == GenState::ERROR || event.state == GenState::ABORT) failed = true;
            if (event.state == GenState::END) ended = true;
            if (event.state == GenState::BEGIN || event.state == GenState::CONTINUE || event.state == GenState::END) {
                const std::string text = (event.text == "<|im_end|>" || event.text == "<|endoftext|>") ? "" : event.text;
                response += text;
                std::cout << text << std::flush;
            }
        }), "Generation");
        std::cout << '\n';
        if (failed || !ended || response.empty()) throw std::runtime_error("Generation did not produce a complete response");
        ProfileData profile;
        if (generator->get_profiler(profile) == ErrorCode::SUCCESS)
            std::cout << "Decode: " << profile.generator.generate_tps << " tokens/s\n";
        std::cout << "COMPLETED\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
