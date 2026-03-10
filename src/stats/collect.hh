// 0.2.0

#ifndef XMDC780935NYX87457TYBNV27Y4X7C6
#define XMDC780935NYX87457TYBNV27Y4X7C6

#include <limits>

namespace sz::stats {

	template <typename T> struct last_total_min_max
	{
			T samples = 0;
			T last_delta = 0; // Invalid if samples == 0
			T total = 0;
			T min = std::numeric_limits<T>::max();
			T max = std::numeric_limits<T>::min();
			T umin = std::numeric_limits<T>::max();
			T umax = 0;

		void reset() {
			samples = 0;
			last_delta = 0;
			total = 0;
			min = std::numeric_limits<T>::max();
			max = std::numeric_limits<T>::min();
			umin = std::numeric_limits<T>::max();
			umax = 0;
		}

		void update(T x) {
			++samples;
			total += (last_delta = x);
			if (x < min) min = x;
			if (x > max) max = x;
			auto abs_x = (x < 0 ? -x : x);
			if (abs_x < umin) umin = abs_x;
			if (abs_x > umax) umax = abs_x;
		}

		auto average() const { return total / samples; }
	};

} // namespace sz::stats

#endif // XMDC780935NYX87457TYBNV27Y4X7C6
