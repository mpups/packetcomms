/**
    Copyright (C) Mark Pupilli 2013, All Rights Reserved
    This file contains utilities that wrap up functionality from the Cereal library.

    Also contains cereal compatible serialize functions for some common types.
*/
#ifndef SERIALISATION_H
#define SERIALISATION_H

#include <time.h>

#include <cereal/archives/binary.hpp>

template<typename ...Args>
void Serialise(std::streambuf& stream, const Args&... types)
{
    std::ostream archiveStream(&stream);
    cereal::BinaryOutputArchive archive(archiveStream);
    archive(std::forward<const Args&>(types)...);
}

template<typename ...Args>
void Deserialise(std::streambuf& vis, Args&... types)
{
    std::istream stream(&vis);
    cereal::BinaryInputArchive archive(stream);
    archive(std::forward<Args&>(types)...);
}

template<typename T>
void save(T& archive, const timespec& t)
{
    const int64_t sec = t.tv_sec;
    const int64_t nsec = t.tv_nsec;
    archive(sec,nsec);
}

template<typename T>
void load(T& archive, timespec& t)
{
    int64_t sec;
    int64_t nsec;
    archive(sec,nsec);
    t.tv_sec = sec;
    t.tv_nsec = nsec;
}

#endif // SERIALISATION_H
