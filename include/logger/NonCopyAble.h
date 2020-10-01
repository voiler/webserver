//
// Created by Yvan on 2020/9/29.
//

#pragma once

class NonCopyAble {
protected:
    NonCopyAble() = default;

    ~NonCopyAble() = default;

public:
    NonCopyAble(const NonCopyAble &) = delete;

    const NonCopyAble &operator=(const NonCopyAble &) = delete;
};