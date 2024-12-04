#pragma once
#ifndef _SHARED_MACROS_H
#ifdef PROJECT_EXPORTS
// https://stackoverflow.com/a/78330763
#if defined(_MSC_VER) && !defined(__clang__)
#define API __declspec(dllexport)
#else
#error "TODO: Add support for non-MSVC compilers with PROJECT_EXPORTS"
#endif
#else
// https://stackoverflow.com/a/78330763
#if defined(_MSC_VER) && !defined(__clang__)
#define API __declspec(dllimport)
#else
#define API __attribute__((visibility("default")))
#define __fastcall __attribute__((fastcall))
#endif
#endif
#endif