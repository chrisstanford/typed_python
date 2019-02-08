#pragma once

template<class T1, class T2>
class PromotesTo {};

template<> class PromotesTo<bool, bool> { public: typedef bool result_type; };
template<> class PromotesTo<int8_t, bool> { public: typedef int8_t result_type; };
template<> class PromotesTo<int16_t, bool> { public: typedef int16_t result_type; };
template<> class PromotesTo<int32_t, bool> { public: typedef int32_t result_type; };
template<> class PromotesTo<int64_t, bool> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint8_t, bool> { public: typedef uint8_t result_type; };
template<> class PromotesTo<uint16_t, bool> { public: typedef uint16_t result_type; };
template<> class PromotesTo<uint32_t, bool> { public: typedef uint32_t result_type; };
template<> class PromotesTo<uint64_t, bool> { public: typedef uint64_t result_type; };
template<> class PromotesTo<bool, int8_t> { public: typedef int8_t result_type; };
template<> class PromotesTo<bool, int16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<bool, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<bool, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<bool, uint8_t> { public: typedef uint8_t result_type; };
template<> class PromotesTo<bool, uint16_t> { public: typedef uint16_t result_type; };
template<> class PromotesTo<bool, uint32_t> { public: typedef uint32_t result_type; };
template<> class PromotesTo<bool, uint64_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<float, bool> { public: typedef float result_type; };
template<> class PromotesTo<float, int8_t> { public: typedef float result_type; };
template<> class PromotesTo<float, int16_t> { public: typedef float result_type; };
template<> class PromotesTo<float, int32_t> { public: typedef float result_type; };
template<> class PromotesTo<float, int64_t> { public: typedef float result_type; };
template<> class PromotesTo<float, uint8_t> { public: typedef float result_type; };
template<> class PromotesTo<float, uint16_t> { public: typedef float result_type; };
template<> class PromotesTo<float, uint32_t> { public: typedef float result_type; };
template<> class PromotesTo<float, uint64_t> { public: typedef float result_type; };
template<> class PromotesTo<double, bool> { public: typedef double result_type; };
template<> class PromotesTo<double, int8_t> { public: typedef double result_type; };
template<> class PromotesTo<double, int16_t> { public: typedef double result_type; };
template<> class PromotesTo<double, int32_t> { public: typedef double result_type; };
template<> class PromotesTo<double, int64_t> { public: typedef double result_type; };
template<> class PromotesTo<double, uint8_t> { public: typedef double result_type; };
template<> class PromotesTo<double, uint16_t> { public: typedef double result_type; };
template<> class PromotesTo<double, uint32_t> { public: typedef double result_type; };
template<> class PromotesTo<double, uint64_t> { public: typedef double result_type; };
template<> class PromotesTo<bool, float> { public: typedef float result_type; };
template<> class PromotesTo<int8_t, float> { public: typedef float result_type; };
template<> class PromotesTo<int16_t, float> { public: typedef float result_type; };
template<> class PromotesTo<int32_t, float> { public: typedef float result_type; };
template<> class PromotesTo<int64_t, float> { public: typedef float result_type; };
template<> class PromotesTo<uint8_t, float> { public: typedef float result_type; };
template<> class PromotesTo<uint16_t, float> { public: typedef float result_type; };
template<> class PromotesTo<uint32_t, float> { public: typedef float result_type; };
template<> class PromotesTo<uint64_t, float> { public: typedef float result_type; };
template<> class PromotesTo<bool, double> { public: typedef double result_type; };
template<> class PromotesTo<int8_t, double> { public: typedef double result_type; };
template<> class PromotesTo<int16_t, double> { public: typedef double result_type; };
template<> class PromotesTo<int32_t, double> { public: typedef double result_type; };
template<> class PromotesTo<int64_t, double> { public: typedef double result_type; };
template<> class PromotesTo<uint8_t, double> { public: typedef double result_type; };
template<> class PromotesTo<uint16_t, double> { public: typedef double result_type; };
template<> class PromotesTo<uint32_t, double> { public: typedef double result_type; };
template<> class PromotesTo<uint64_t, double> { public: typedef double result_type; };
template<> class PromotesTo<int8_t, int8_t> { public: typedef int8_t result_type; };
template<> class PromotesTo<int16_t, int8_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<int8_t, int16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<int16_t, int16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<int32_t, int8_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int8_t, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int32_t, int16_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int16_t, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int32_t, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int64_t, int8_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int8_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int64_t, int16_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int64_t, int32_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int16_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int32_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int64_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint8_t, int8_t> { public: typedef int8_t result_type; };
template<> class PromotesTo<uint16_t, int8_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<uint8_t, int16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<uint16_t, int16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<uint32_t, int8_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<uint8_t, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<uint32_t, int16_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<uint16_t, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<uint32_t, int32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<uint64_t, int8_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint8_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint64_t, int16_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint64_t, int32_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint16_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint32_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint64_t, int64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int8_t, uint8_t> { public: typedef int8_t result_type; };
template<> class PromotesTo<int16_t, uint8_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<int8_t, uint16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<int16_t, uint16_t> { public: typedef int16_t result_type; };
template<> class PromotesTo<int32_t, uint8_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int8_t, uint32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int32_t, uint16_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int16_t, uint32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int32_t, uint32_t> { public: typedef int32_t result_type; };
template<> class PromotesTo<int64_t, uint8_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int8_t, uint64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int64_t, uint16_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int64_t, uint32_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int16_t, uint64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int32_t, uint64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<int64_t, uint64_t> { public: typedef int64_t result_type; };
template<> class PromotesTo<uint8_t, uint8_t> { public: typedef uint8_t result_type; };
template<> class PromotesTo<uint16_t, uint8_t> { public: typedef uint16_t result_type; };
template<> class PromotesTo<uint8_t, uint16_t> { public: typedef uint16_t result_type; };
template<> class PromotesTo<uint16_t, uint16_t> { public: typedef uint16_t result_type; };
template<> class PromotesTo<uint32_t, uint8_t> { public: typedef uint32_t result_type; };
template<> class PromotesTo<uint8_t, uint32_t> { public: typedef uint32_t result_type; };
template<> class PromotesTo<uint32_t, uint16_t> { public: typedef uint32_t result_type; };
template<> class PromotesTo<uint16_t, uint32_t> { public: typedef uint32_t result_type; };
template<> class PromotesTo<uint32_t, uint32_t> { public: typedef uint32_t result_type; };
template<> class PromotesTo<uint64_t, uint8_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<uint8_t, uint64_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<uint64_t, uint16_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<uint64_t, uint32_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<uint16_t, uint64_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<uint32_t, uint64_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<uint64_t, uint64_t> { public: typedef uint64_t result_type; };
template<> class PromotesTo<float, double> { public: typedef double result_type; };
template<> class PromotesTo<double, float> { public: typedef double result_type; };
template<> class PromotesTo<float, float> { public: typedef float result_type; };
template<> class PromotesTo<double, double> { public: typedef double result_type; };





