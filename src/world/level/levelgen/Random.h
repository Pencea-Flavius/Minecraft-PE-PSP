#ifndef RANDOM_H__
#define RANDOM_H__

#include <cmath>

class Random
{
public:
	Random() {
		setSeed( 0 );
	}
	Random( long seed ) {
		setSeed( seed );
	}

	void setSeed( long seed ) {
		_seed = seed;
		_mti = N + 1;
		haveNextNextGaussian = false;
		nextNextGaussian = 0;
		init_genrand(seed);
	}
	long getSeed() {
		return _seed;
}
	bool nextBoolean() {
		return (genrand_int32() & 0x8000000) > 0;
	}
	float nextFloat() {
		return (float)genrand_real2();
	}
	double nextDouble() {
		return genrand_real2();
	}
	int nextInt() {
		return (int)(genrand_int32()>>1);
	}
	int nextInt(int n) {
		return genrand_int32() % n;
	}
	int  nextLong() {
		return (int)(genrand_int32()>>1);
	}
	int  nextLong(int  n) {
		return genrand_int32() % n;
	}

	float nextGaussian()
	{
		if (haveNextNextGaussian) {
			haveNextNextGaussian = false;
			return nextNextGaussian;
		} else {
			float v1, v2, s;
			do {
				v1 = 2 * nextFloat() - 1;
				v2 = 2 * nextFloat() - 1;
				s = v1 * v1 + v2 * v2;
			} while (s >= 1 || s == 0);
			float multiplier = std::sqrt(-2 * std::log(s)/s);
			nextNextGaussian = v2 * multiplier;
			haveNextNextGaussian = true;
			return v1 * multiplier;
		}
	}
private:
	long _seed;

	static const int N = 624;
	static const int M = 397;
	static const unsigned int MATRIX_A = 0x9908b0dfUL;
	static const unsigned int UPPER_MASK = 0x80000000UL;
	static const unsigned int LOWER_MASK = 0x7fffffffUL;

	unsigned long _mt[N];
	int _mti;

	bool haveNextNextGaussian;
	float nextNextGaussian;

	void init_genrand(unsigned long s)
	{
		_mt[0] = s & 0xffffffffUL;
		for (_mti=1; _mti < N; _mti++) {
			_mt[_mti] =
			(1812433253UL * (_mt[_mti-1] ^ (_mt[_mti-1] >> 30)) + _mti);
			_mt[_mti] &= 0xffffffffUL;
		}
	}

	unsigned long genrand_int32(void)
	{
		unsigned long y;
		static unsigned long mag01[2]={0x0UL, MATRIX_A};

		if (_mti >= N) {
			int kk;

			if (_mti == N+1)
				init_genrand(5489UL);

			for (kk=0;kk<N-M;kk++) {
				y = (_mt[kk]&UPPER_MASK)|(_mt[kk+1]&LOWER_MASK);
				_mt[kk] = _mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
			}
			for (;kk<N-1;kk++) {
				y = (_mt[kk]&UPPER_MASK)|(_mt[kk+1]&LOWER_MASK);
				_mt[kk] = _mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
			}
			y = (_mt[N-1]&UPPER_MASK)|(_mt[0]&LOWER_MASK);
			_mt[N-1] = _mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

			_mti = 0;
		}

		y = _mt[_mti++];

		y ^= (y >> 11);
		y ^= (y << 7) & 0x9d2c5680UL;
		y ^= (y << 15) & 0xefc60000UL;
		y ^= (y >> 18);

		return y;
	}

	double genrand_real2(void)
	{
		return genrand_int32()*(1.0/4294967296.0);

	}
};

#endif
