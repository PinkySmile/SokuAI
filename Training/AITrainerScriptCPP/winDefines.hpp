//
// Created by PinkySmile on 22/10/24.
//

#ifndef SOKUAI_WINDEFINES_HPP
#define SOKUAI_WINDEFINES_HPP


#include <cmath>
#include <cstring>

#define __argv __libc_argv
extern char **__libc_argv;

typedef int SOCKET;
#define INVALID_SOCKET -1
#define strcpy_s(src, dest) strcpy(src, dest)
#define SocketError LinuxException
#define closesocket(sock) close(sock)
typedef fd_set FD_SET;
using std::max;
using std::min;

namespace SokuLib {
	enum Direction : char {
		LEFT = -1,
		RIGHT = 1
	};

	struct Skill {
		unsigned char level : 7;
		bool notUsed : 1;
	};

	enum Character : unsigned int {
		/* 0  */ CHARACTER_REIMU,
		/* 1  */ CHARACTER_MARISA,
		/* 2  */ CHARACTER_SAKUYA,
		/* 3  */ CHARACTER_ALICE,
		/* 4  */ CHARACTER_PATCHOULI,
		/* 5  */ CHARACTER_YOUMU,
		/* 6  */ CHARACTER_REMILIA,
		/* 7  */ CHARACTER_YUYUKO,
		/* 8  */ CHARACTER_YUKARI,
		/* 9  */ CHARACTER_SUIKA,
		/* 10 */ CHARACTER_REISEN,
		/* 10 */ CHARACTER_UDONGE = CHARACTER_REISEN,
		/* 11 */ CHARACTER_AYA,
		/* 12 */ CHARACTER_KOMACHI,
		/* 13 */ CHARACTER_IKU,
		/* 14 */ CHARACTER_TENSHI,
		/* 15 */ CHARACTER_SANAE,
		/* 16 */ CHARACTER_CIRNO,
		/* 16 */ CHARACTER_CHIRNO = CHARACTER_CIRNO,
		/* 17 */ CHARACTER_MEILING,
		/* 17 */ CHARACTER_MEIRIN = CHARACTER_MEILING,
		/* 18 */ CHARACTER_UTSUHO,
		/* 19 */ CHARACTER_SUWAKO,
		/* 20 */ CHARACTER_RANDOM,
		/* 21 */ CHARACTER_NAMAZU,
		//Soku2 Characters
		/* 22 */ CHARACTER_MOMIJI,
		/* 23 */ CHARACTER_CLOWNPIECE,
		/* 24 */ CHARACTER_FLANDRE,
		/* 25 */ CHARACTER_ORIN,
		/* 26 */ CHARACTER_YUUKA,
		/* 27 */ CHARACTER_KAGUYA,
		/* 28 */ CHARACTER_MOKOU,
		/* 29 */ CHARACTER_MIMA,
		/* 30 */ CHARACTER_SHOU,
		/* 31 */ CHARACTER_MURASA,
		/* 32 */ CHARACTER_SEKIBANKI,
		/* 33 */ CHARACTER_SATORI,
		/* 34 */ CHARACTER_RAN,
		/* 35 */ CHARACTER_TEWI
	};

	template<typename T>
	struct Vector2 {
		T x;
		T y;

		template<typename T2>
		bool operator==(const Vector2<T2> &other) const
		{
			return this->x == other.x && this->y == other.y;
		}

		template<typename T2>
		Vector2<T> &operator-=(Vector2<T2> other)
		{
			this->x -= other.x;
			this->y -= other.y;
			return *this;
		}

		template<typename T2>
		Vector2<T> operator-(const Vector2<T2> &other) const
		{
			return {
				static_cast<T>(this->x - other.x),
				static_cast<T>(this->y - other.y)
			};
		}

		template<typename T2>
		Vector2<T> operator-(T2 other) const
		{
			return {
				static_cast<T>(this->x - other),
				static_cast<T>(this->y - other)
			};
		}

		template<typename T2>
		Vector2<T> &operator+=(Vector2<T2> other)
		{
			this->x += other.x;
			this->y += other.y;
			return *this;
		}

		template<typename T2>
		Vector2<T> operator+(const Vector2<T2> &other) const
		{
			return {
				static_cast<T>(this->x + other.x),
				static_cast<T>(this->y + other.y)
			};
		}

		template<typename T2>
		Vector2<T> &operator*=(T2 scalar)
		{
			this->x *= scalar;
			this->y *= scalar;
			return *this;
		}

		template<typename T2>
		auto operator*(T2 scalar) const
		{
			return Vector2<decltype(this->x * scalar)>{
				this->x * scalar,
				this->y * scalar
			};
		}

		template<typename T2>
		Vector2<T> &operator/=(T2 scalar)
		{
			this->x /= scalar;
			this->y /= scalar;
			return *this;
		}

		template<typename T2>
		auto operator/(T2 scalar) const
		{
			return Vector2<decltype(this->x / scalar)>{
				this->x / scalar,
				this->y / scalar
			};
		}

		Vector2<float> rotate(float angle, const Vector2<T> &center) const noexcept
		{
			if (angle == 0.f)
				return {
					static_cast<float>(this->x),
					static_cast<float>(this->y)
				};

			float c = cos(angle);
			float s = sin(angle);

			Vector2<float> result{
				static_cast<float>(c * (static_cast<float>(this->x) - center.x) - s * (static_cast<float>(this->y) - center.y) + center.x),
				static_cast<float>(s * (static_cast<float>(this->x) - center.x) + c * (static_cast<float>(this->y) - center.y) + center.y)
			};

			return result;
		}

		template<typename T2>
		Vector2<T2> to() const
		{
			return {
				static_cast<T2>(this->x),
				static_cast<T2>(this->y)
			};
		}
	};

	typedef Vector2<float>              Vector2f;
	typedef Vector2<int>                Vector2i;
	typedef Vector2<unsigned>           Vector2u;
	typedef Vector2<long>               Vector2l;
	typedef Vector2<unsigned long>      Vector2ul;
	typedef Vector2<long long>          Vector2ll;
	typedef Vector2<unsigned long long> Vector2ull;
	typedef Vector2<double>             Vector2d;
	typedef Vector2<bool>               Vector2b;
}

#endif //SOKUAI_WINDEFINES_HPP
