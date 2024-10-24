#pragma once

#include <stdio.h>
#include <stdint.h>
#include <type_traits> // std::is_unsigned
#include <limits>  // std::numeric_limits
#include <math.h> // isinf
// From https://tttapa.github.io/Pages/Mathematics/Systems-and-Control-Theory/Digital-filters/Exponential%20Moving%20Average/C++Implementation.html

/**
 * @brief   Exponential moving average filter.
 *
 * Fast integer EMA implementation where the weight factor is a power of two.
 *
 * Difference equation: @f$ y[n] = \alpha·x[n]+(1-\alpha)·y[n-1] @f$
 * where @f$ \alpha = \left(\frac{1}{2}\right)^{K} @f$, @f$ x @f$ is the
 * input sequence, and @f$ y @f$ is the output sequence.
 *
 * [An in-depth explanation of the EMA filter](https://tttapa.github.io/Pages/Mathematics/Systems-and-Control-Theory/Digital-filters/Exponential%20Moving%20Average/)
 *
 * @tparam  K
 *  The amount of bits to shift by. This determines the location
 *  of the pole in the EMA transfer function, and therefore the
 *  cut-off frequency.
 *  The higher this number, the more filtering takes place.
 *  The pole location is @f$ 1 - 2^{-K} @f$.
 * @tparam  input_t
 *  The integer type to use for the input and output of the filter.
 *  Can be signed or unsigned.
 * @tparam  state_t
 *  The unsigned integer type to use for the internal state of the
 *  filter. A fixed-point representation with @f$ K @f$ fractional
 *  bits is used, so this type should be at least @f$ M + K @f$ bits
 *  wide, where @f$ M @f$ is the maximum number of bits of the input.
 *
 * Some examples of different combinations of template parameters:
 *
 * 1. Filtering the result of `analogRead`: analogRead returns an integer
 *between 0 and 1023, which can be represented using 10 bits, so
 *@f$ M = 10 @f$. If `input_t` and `output_t` are both `uint16_t`,
 *the maximum shift factor `K` is @f$ 16 - M = 6 @f$. If `state_t`
 *is increased to `uint32_t`, the maximum shift factor `K` is
 *@f$ 32 - M = 22 @f$.
 * 2. Filtering a signed integer between -32768 and 32767: this can be
 *represented using a 16-bit signed integer, so `input_t` is `int16_t`,
 *and @f$ M = 16 @f$. (2¹⁵ = 32768)
 *Let's say the shift factor `K` is 1, then the minimum width of
 *`state_t` should be @f$ M + K = 17 @f$ bits, so `uint32_t` would be
 *a sensible choice.
 */
template <uint8_t K,
          class input_t = uint_fast16_t,
          class state_t = std::make_unsigned_t<input_t>>
class EMA
{
public:
/// Constructor: initialize filter to zero or optional given value.
    EMA(input_t initial = input_t(0))
        : state(zero + (state_t(initial) << K) - initial) {}

/// Update the filter with the given input and return the filtered output.
    input_t operator()(input_t input)
    {
        state += state_t(input);
        state_t output = (state + half) >> K;
        output-= zero >> K;
        state -= output;
        return input_t(output);
    }

    static constexpr state_t
    max_state  = std::numeric_limits<state_t>::max(),
    half_state = max_state / 2 + 1,
    zero   = std::is_unsigned<input_t>::value ? state_t(0) : half_state,
    half   = K > 0 ? state_t(1) << (K - 1) : state_t(0);

    static_assert(std::is_unsigned<state_t>::value,
                  "state type should be unsigned");

    static_assert(max_state >= std::numeric_limits<input_t>::max(),
                  "state type cannot be narrower than input type");

/// Verify the input range to make sure it's compatible with the shift
/// factor and the width of the state type.
    template <class T>
    static constexpr bool supports_range(T min, T max)
    {
        using sstate_t = std::make_signed_t<state_t>;
        return min <= max &&
               min >= std::numeric_limits<input_t>::min() &&
               max <= std::numeric_limits<input_t>::max() &&
               (std::is_unsigned<input_t>::value
                ? state_t(max) <= (max_state >> K)
                : min >= -sstate_t(max_state >> (K + 1)) - 1 &&
                max <= sstate_t(max_state >> (K + 1)));
    }

private:
    state_t state;
};

// Simpel filter
// EMA filter{1.5};
// std::cout<< "1 " <<filter(3) << "\n";
// std::cout<< "1 " <<filter(4) << "\n";
// std::cout<< "1 " <<filter(5) << "\n";
class EMAFloat
{

public:
// Constructor
    EMAFloat(float K) : K(K), state(0) {}

/// Update the filter with the given input and return the filtered output.
    float operator()(float input)
    {
        state += input;
        float output = (state + 1.5f) / K;
        state -= output;
        return output;
    }

private:
    const float K;
    float state;
};


template <uint8_t DP>
class RatePerSecond
{
    EMAFloat filter{1};
//    etl::circular_buffer<float, DP> buffer;;
    float previous;
    float delta;
public:
    RatePerSecond(float K) : filter{EMAFloat{K}}, previous{0}, delta{0.0}
    {

    }

    /**
     * Adds a new value and returns the filtered delta from the previous value
    */
    float operator()(float input)
    {
        if (isinf(input))
        {
            return 0.f;
        }
        //      buffer.push(input);
        //    float delta = input - buffer.front();
        float newValue = filter(input);
        delta = newValue - previous;
        previous = newValue;
        return delta;
    }

    /**
     * Returns the filtered value
    */
    float operator()() const
    {
//        printf("previous: %.2f\n", previous);
        return previous;
    }

    /**
     * Returns the perSecond delta
    */
    inline float perSecond() const
    {
        return delta / (1.0f / (float)DP);
    }



    // float perSec() {
    //     return ;
    // }

    // float operator()()
    // {
    //     return current;
    // }

    // void print() {
    //     for (auto f : buffer) {
    //         printf("%.2f ", f);
    //     }
    //     printf("\n");
    // }
};
