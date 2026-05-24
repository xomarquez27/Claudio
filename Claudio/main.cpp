//
//  main.cpp
//  Claudio
//
//  Created by Xavier Otero Marquez on 5/23/26.
//

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Event {
    double frequency;
    double durationSeconds;
    bool isRest;
};

class ClaudioInput {
public:
    static std::vector<std::string> tokensFromArguments(int argc, const char* argv[]) {
        std::vector<std::string> args;

        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }

        if (args.empty()) {
            return defaultTokens();
        }

        if (args.size() == 1 && looksLikeTextFile(args[0])) {
            return tokensFromFile(args[0]);
        }

        if (args.size() == 2 && args[0] == "-f") {
            return tokensFromFile(args[1]);
        }

        return args;
    }

private:
    static std::vector<std::string> defaultTokens() {
        // Xcode usually runs command-line apps without arguments.
        // This gives us an audible default phrase while developing.
        return {"t-180", "4/4", "-8", "e5", "e5", "r", "e5", "r", "c5", "-4", "e5", "g5", "r", "g4", "r"};
    }

    static bool looksLikeTextFile(const std::string& path) {
        return path.size() >= 4 && path.substr(path.size() - 4) == ".txt";
    }

    static std::vector<std::string> tokensFromFile(const std::string& path) {
        std::ifstream file(path);
        std::vector<std::string> tokens;

        if (!file) {
            std::cerr << "Could not open file: " << path << '\n';
            return tokens;
        }

        std::string line;
        while (std::getline(file, line)) {
            const std::size_t commentPosition = line.find('#');
            if (commentPosition != std::string::npos) {
                line = line.substr(0, commentPosition);
            }

            std::istringstream stream(line);
            std::string token;
            while (stream >> token) {
                tokens.push_back(token);
            }
        }

        return tokens;
    }
};

class ClaudioParser {
public:
    std::vector<Event> parse(const std::vector<std::string>& tokens) const {
        int bpm = 120;
        int currentDuration = 4;
        std::vector<Event> events;

        for (const std::string& rawToken : tokens) {
            const std::string token = lowercase(rawToken);

            if (startsWith(token, "t-")) {
                bpm = std::stoi(token.substr(2));
                continue;
            }

            if (isTimeSignature(token)) {
                // Time signature placeholder. Currently accepted but not enforced.
                continue;
            }

            int parsedDuration = currentDuration;
            if (parseDurationToken(token, parsedDuration)) {
                currentDuration = parsedDuration;
                continue;
            }

            std::string notePart;
            int overrideDuration = currentDuration;
            const bool hasOverride = splitOverride(token, notePart, overrideDuration);
            const int eventDuration = hasOverride ? overrideDuration : currentDuration;
            const double eventSeconds = secondsForDuration(eventDuration, bpm);

            if (notePart == "r") {
                events.push_back({0.0, eventSeconds, true});
                continue;
            }

            double frequency = 0.0;
            if (noteToFrequency(notePart, frequency)) {
                events.push_back({frequency, eventSeconds, false});
                continue;
            }

            std::cerr << "Ignoring unknown token: " << rawToken << '\n';
        }

        return events;
    }

private:
    static bool startsWith(const std::string& value, const std::string& prefix) {
        return value.rfind(prefix, 0) == 0;
    }

    static std::string lowercase(std::string value) {
        for (char& character : value) {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        return value;
    }

    static bool isTimeSignature(const std::string& token) {
        return token.find('/') != std::string::npos
            && token.size() >= 3
            && std::isdigit(static_cast<unsigned char>(token[0]))
            && token.find_first_not_of("0123456789/") == std::string::npos;
    }

    static double beatsForDuration(int denominator) {
        return 4.0 / static_cast<double>(denominator);
    }

    static double secondsForDuration(int denominator, int bpm) {
        const double secondsPerBeat = 60.0 / static_cast<double>(bpm);
        return beatsForDuration(denominator) * secondsPerBeat;
    }

    static bool parseDurationToken(const std::string& token, int& duration) {
        if (token.size() < 2 || token[0] != '-') {
            return false;
        }

        try {
            const int parsed = std::stoi(token.substr(1));
            if (isSupportedDuration(parsed)) {
                duration = parsed;
                return true;
            }
        } catch (...) {
            return false;
        }

        return false;
    }

    static bool splitOverride(const std::string& token, std::string& notePart, int& durationOverride) {
        const std::size_t slashPosition = token.find('/');

        if (slashPosition == std::string::npos) {
            notePart = token;
            return false;
        }

        notePart = token.substr(0, slashPosition);
        const std::string durationPart = token.substr(slashPosition + 1);

        try {
            const int parsed = std::stoi(durationPart);
            if (isSupportedDuration(parsed)) {
                durationOverride = parsed;
                return true;
            }
        } catch (...) {
            return false;
        }

        return false;
    }

    static bool isSupportedDuration(int duration) {
        return duration == 1 || duration == 2 || duration == 4 || duration == 8 || duration == 16 || duration == 32;
    }

    static bool noteToFrequency(const std::string& rawNote, double& frequency) {
        const std::string note = lowercase(rawNote);

        if (note.size() < 2) {
            return false;
        }

        static const std::map<std::string, int> semitones = {
            {"c", 0}, {"c#", 1}, {"db", 1},
            {"d", 2}, {"d#", 3}, {"eb", 3},
            {"e", 4},
            {"f", 5}, {"f#", 6}, {"gb", 6},
            {"g", 7}, {"g#", 8}, {"ab", 8},
            {"a", 9}, {"a#", 10}, {"bb", 10},
            {"b", 11}
        };

        std::string pitchName;
        std::string octaveText;

        pitchName += note[0];

        if (note.size() >= 3 && (note[1] == '#' || note[1] == 'b')) {
            pitchName += note[1];
            octaveText = note.substr(2);
        } else {
            octaveText = note.substr(1);
        }

        if (semitones.find(pitchName) == semitones.end()) {
            return false;
        }

        try {
            const int octave = std::stoi(octaveText);
            const int midiNote = 12 * (octave + 1) + semitones.at(pitchName);
            frequency = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
            return true;
        } catch (...) {
            return false;
        }
    }
};

class ClaudioAudio {
public:
    bool play(const std::vector<Event>& events) const {
        const std::string outputPath = "/tmp/claudio.wav";
        writeWav(outputPath, events);

        std::cout << "Wrote " << events.size() << " events to " << outputPath << '\n';
        std::cout << "Playing with afplay...\n";

        const std::string command = "afplay " + outputPath;
        const int result = std::system(command.c_str());

        if (result != 0) {
            std::cerr << "afplay failed. Try opening /tmp/claudio.wav manually.\n";
            return false;
        }

        return true;
    }

private:
    static void writeLittleEndian(std::ofstream& file, std::uint32_t value, int byteCount) {
        for (int i = 0; i < byteCount; ++i) {
            file.put(static_cast<char>((value >> (8 * i)) & 0xFF));
        }
    }

    static void writeWav(const std::string& path, const std::vector<Event>& events) {
        constexpr int sampleRate = 44100;
        constexpr int bitsPerSample = 16;
        constexpr int channels = 1;
        constexpr double amplitude = 0.25;

        std::vector<std::int16_t> samples;

        for (const Event& event : events) {
            const int sampleCount = static_cast<int>(event.durationSeconds * sampleRate);

            for (int i = 0; i < sampleCount; ++i) {
                double sample = 0.0;

                if (!event.isRest) {
                    const double t = static_cast<double>(i) / static_cast<double>(sampleRate);
                    sample = std::sin(2.0 * M_PI * event.frequency * t);

                    const int fadeSamples = 200;
                    if (i < fadeSamples) {
                        sample *= static_cast<double>(i) / fadeSamples;
                    } else if (sampleCount - i < fadeSamples) {
                        sample *= static_cast<double>(sampleCount - i) / fadeSamples;
                    }
                }

                samples.push_back(static_cast<std::int16_t>(sample * amplitude * INT16_MAX));
            }
        }

        std::ofstream file(path, std::ios::binary);

        const std::uint32_t dataSize = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
        const std::uint32_t riffSize = 36 + dataSize;
        const std::uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
        const std::uint16_t blockAlign = channels * bitsPerSample / 8;

        file.write("RIFF", 4);
        writeLittleEndian(file, riffSize, 4);
        file.write("WAVE", 4);

        file.write("fmt ", 4);
        writeLittleEndian(file, 16, 4);
        writeLittleEndian(file, 1, 2);
        writeLittleEndian(file, channels, 2);
        writeLittleEndian(file, sampleRate, 4);
        writeLittleEndian(file, byteRate, 4);
        writeLittleEndian(file, blockAlign, 2);
        writeLittleEndian(file, bitsPerSample, 2);

        file.write("data", 4);
        writeLittleEndian(file, dataSize, 4);
        file.write(reinterpret_cast<const char*>(samples.data()), dataSize);
    }
};

int main(int argc, const char* argv[]) {
    const std::vector<std::string> tokens = ClaudioInput::tokensFromArguments(argc, argv);

    const ClaudioParser parser;
    const std::vector<Event> events = parser.parse(tokens);

    if (events.empty()) {
        std::cerr << "No playable notes found.\n";
        return EXIT_FAILURE;
    }

    const ClaudioAudio audio;
    return audio.play(events) ? EXIT_SUCCESS : EXIT_FAILURE;
}
