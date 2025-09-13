#include "SubtitleFile.h"
#include <fstream>
#include <algorithm>
#include <filesystem>

bool IsLittleEndian() {
    union {
        uint32_t i;
        char c[4];
    } test = { 0x01020304 };
    return test.c[0] == 0x04;
}

void SubtitleFile::ReadLsbFile(const std::string& path) {
    badLSB = false;
    subtitles.clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        badLSB = true;
        return;
    }

    uint16_t keyValCount = ReadUShort(stream);
    uint32_t value_offset = ReadUInt(stream);

    std::vector<uint8_t> key_lengths(keyValCount);
    stream.read(reinterpret_cast<char*>(key_lengths.data()), keyValCount);

    std::vector<uint16_t> value_lengths;
    for (int i = 0; i < keyValCount; ++i) {
        value_lengths.push_back(ReadUShort(stream));
    }

    stream.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(stream.tellg()) - (2 + 4 + keyValCount + keyValCount * 2);
    stream.seekg(2 + 4 + keyValCount + keyValCount * 2, std::ios::beg);
    std::vector<uint8_t> raw_data(size);
    stream.read(reinterpret_cast<char*>(raw_data.data()), size);
    std::vector<uint8_t> decrypted = XORCrypt(raw_data);

    size_t key_offset = 0;
    try {
        for (size_t i = 0; i < keyValCount; ++i) {
            std::vector<uint8_t> key(key_lengths[i]);
            std::vector<uint8_t> value(value_lengths[i]);
            std::copy(decrypted.begin() + key_offset, decrypted.begin() + key_offset + key_lengths[i], key.begin());
            key_offset += key_lengths[i];
            std::copy(decrypted.begin() + value_offset, decrypted.begin() + value_offset + value_lengths[i], value.begin());
            value_offset += value_lengths[i];
            subtitles[std::string(key.begin(), key.end())] = std::string(value.begin(), value.end());
        }
    }
    catch (...) {
        badLSB = true;
    }
    stream.close();
}

void SubtitleFile::ReadTxtFile(const std::string& path) {
    subtitles.clear();
    std::ifstream stream(path);
    std::string line;
    while (std::getline(stream, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            subtitles[key] = value;
        }
    }
    stream.close();
}

void SubtitleFile::WriteLsbFile(const std::string& dest_path) {
    std::ofstream memstream(dest_path + ".tmp", std::ios::binary);
    std::vector<uint8_t> key_lengths;
    std::vector<uint16_t> value_lengths;
    std::string keys_concat, values_concat;

    // Write header
    WriteUShort(memstream, static_cast<uint16_t>(subtitles.size()));
    for (const auto& [key, value] : subtitles) {
        keys_concat += key;
        values_concat += value;
        key_lengths.push_back(static_cast<uint8_t>(key.length()));
        value_lengths.push_back(static_cast<uint16_t>(value.length()));
    }
    WriteUInt(memstream, static_cast<uint32_t>(keys_concat.length()));


    memstream.write(reinterpret_cast<char*>(key_lengths.data()), key_lengths.size());


    for (uint16_t len : value_lengths) {
        WriteUShort(memstream, len);
    }

    std::string data = keys_concat + values_concat;
    std::vector<uint8_t> data_bytes(data.begin(), data.end());
    std::vector<uint8_t> enc_data = XORCrypt(data_bytes);
    memstream.write(reinterpret_cast<char*>(enc_data.data()), enc_data.size());
    memstream.close();

    std::string final_path = std::filesystem::path(dest_path).replace_extension(".lsb").string();

    std::filesystem::rename(dest_path + ".tmp", final_path);
}

void SubtitleFile::WriteTxtFile(const std::string& dest_path, bool isUnix) {
    std::string final_path = std::filesystem::path(dest_path).replace_extension(".txt").string();
    if (std::filesystem::exists(final_path)) {
        std::filesystem::rename(final_path, final_path + ".bkp");
    }
    std::ofstream stream(final_path);
    for (const auto& [key, value] : subtitles) {
        stream << key << "=" << value << (isUnix ? "\n" : "\r\n");
    }
    stream.close();
}

uint32_t SubtitleFile::ReadUInt(std::ifstream& stream) {
    uint8_t rawBytes[4];
    stream.read(reinterpret_cast<char*>(rawBytes), 4);
    if (!IsLittleEndian()) {
        std::reverse(rawBytes, rawBytes + 4);
    }
    return *reinterpret_cast<uint32_t*>(rawBytes);
}

uint16_t SubtitleFile::ReadUShort(std::ifstream& stream) {
    uint8_t rawBytes[2];
    stream.read(reinterpret_cast<char*>(rawBytes), 2);
    if (!IsLittleEndian()) {
        std::reverse(rawBytes, rawBytes + 2);
    }
    return *reinterpret_cast<uint16_t*>(rawBytes);
}

void SubtitleFile::WriteUInt(std::ofstream& stream, uint32_t number) {
    uint8_t bytes[4];
    *reinterpret_cast<uint32_t*>(bytes) = number;
    if (!IsLittleEndian()) {
        std::reverse(bytes, bytes + 4);
    }
    stream.write(reinterpret_cast<char*>(bytes), 4);
}

void SubtitleFile::WriteUShort(std::ofstream& stream, uint16_t number) {
    uint8_t bytes[2];
    *reinterpret_cast<uint16_t*>(bytes) = number;
    if (!IsLittleEndian()) {
        std::reverse(bytes, bytes + 2);
    }
    stream.write(reinterpret_cast<char*>(bytes), 2);
}

std::vector<uint8_t> SubtitleFile::XORCrypt(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> content(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        content[i] = data[i] ^ s_key[i % s_key.size()];
    }
    return content;
}