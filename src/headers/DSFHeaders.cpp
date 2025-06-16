//
// Created by Hector van der Aa on 6/14/25.
//

#include "DSFHeaders.h"

#include <fstream>

#include "../lib/EndianHelpers.h"

void sk::headers::DSF::DSDHeader::read(std::ifstream &file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_le<decltype(ChunkSize)>(file);
    FileSize = sk::endian::read_le<decltype(FileSize)>(file);
    MetaPtr = sk::endian::read_le<decltype(MetaPtr)>(file);
}


void sk::headers::DSF::DSDHeader::write(std::ofstream &file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_le<decltype(ChunkSize)>(file, ChunkSize);
    sk::endian::write_le<decltype(FileSize)>(file, FileSize);
    sk::endian::write_le<decltype(MetaPtr)>(file, MetaPtr);
}

void sk::headers::DSF::FMTHeader::read(std::ifstream &file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_le<decltype(ChunkSize)>(file);
    Format = sk::endian::read_le<decltype(Format)>(file);
    FormatID = sk::endian::read_le<decltype(FormatID)>(file);
    ChanType = sk::endian::read_le<decltype(ChanType)>(file);
    ChanNum = sk::endian::read_le<decltype(ChanNum)>(file);
    SampleRate = sk::endian::read_le<decltype(SampleRate)>(file);
    BitsPerSample = sk::endian::read_le<decltype(BitsPerSample)>(file);
    NumSamples = sk::endian::read_le<decltype(NumSamples)>(file);
    BlockSize = sk::endian::read_le<decltype(BlockSize)>(file);
    Reserved = sk::endian::read_le<decltype(Reserved)>(file);
}