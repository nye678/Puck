#ifndef __PUCK_MATH_H_
#define __PUCK_MATH_H_

#include <string.h>
#include <math.h>

namespace puck
{
	struct vec2
	{
		union
		{
			float v[2];
			struct
			{
				float x, y;
			};
		};

		inline float length() const
		{
			return sqrt(x * x + y * y);
		}

		inline vec2() = default;
		inline vec2(float x, float y) : x(x), y(y) {}
	};

	inline vec2 operator+ (const vec2 &lhs, const vec2 &rhs)
	{
		return vec2(lhs.x + rhs.x, lhs.y + rhs.y);
	}

	inline vec2 operator- (const vec2 &lhs, const vec2 &rhs)
	{
		return vec2(lhs.x - rhs.x, lhs.y - rhs.y);	
	}

	inline vec2 operator* (const vec2 &lhs, const float rhs)
	{
		return vec2(lhs.x * rhs, lhs.x * rhs);
	}

	inline vec2 operator/ (const vec2 &lhs, const float rhs)
	{
		return vec2(lhs.x / rhs, lhs.x / rhs);
	}

	inline vec2 normalize(const vec2 &vec)
	{
		return vec / vec.length();
	}

	struct vec3
	{
		union
		{
			float v[3];
			struct
			{
				float x, y, z;
			};
		};

		inline float length() const
		{
			return sqrt(x * x + y * y + z * z);
		}
		
		inline vec3() = default;
		inline vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	};

	inline vec3 operator+ (const vec3 &lhs, const vec3 &rhs)
	{
		return vec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	}

	inline vec3 operator- (const vec3 &lhs, const vec3 &rhs)
	{
		return vec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);	
	}

	inline vec3 operator* (const vec3 &lhs, const float rhs)
	{
		return vec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
	}

	inline vec3 operator/ (const vec3 &lhs, const float rhs)
	{
		return vec3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
	}

	inline vec3 normalize(const vec3 &vec)
	{
		return vec / vec.length();
	}

	struct vec4
	{
		union
		{
			float v[4];
			struct
			{
				float x, y, z, w;
			};
		};

		inline vec4() = default;
		inline vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	};

	inline vec4 operator+ (const vec4 &lhs, const vec4 &rhs)
	{
		return vec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
	}

	inline vec4 operator- (const vec4 &lhs, const vec4 &rhs)
	{
		return vec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w + rhs.w);	
	}

	inline vec4 operator* (const vec4 &lhs, const float rhs)
	{
		return vec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
	}

	inline vec4 operator/ (const vec4 &lhs, const float rhs)
	{
		return vec4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
	}

	struct mat4
	{
		union
		{
			float data[16];
			vec4 column[4];
		};

		inline mat4() = default;
		inline mat4(float n) 
		{ 
			memset(data, 0, sizeof(float) * 16);
			column[0].x = n;
			column[1].y = n;
			column[2].z = n;
			column[3].w = 1.0f;
		}
	};

	mat4 OrthoProjection(float left, float right, float top, float bottom, float znear, float zfar)
	{
		mat4 ortho = {0};
		ortho.column[0].x = 2.0f / (right - left);
		ortho.column[1].y = 2.0f / (top - bottom);
		ortho.column[2].z = 2.0f / (znear - zfar);
		ortho.column[3] = vec4(
			(left + right) / (left- right),
			(bottom + top) / (bottom - top),
			(znear + zfar) / (zfar - znear),
			1.0f);
		return ortho;
	}

	struct rect
	{
		union
		{
			float r[4];
			struct
			{
				float top, left, bottom, right;
			};
			struct
			{
				vec2 topLeft;
				vec2 bottomRight;
			};
		};
		
		inline rect() = default;
		inline rect(float left, float right, float top, float bottom)
			: left(left), right(right), top(top), bottom(bottom) {}
	};

	bool RectIntersection(const rect &r1, const rect &r2)
	{
		return !(r2.left > r1.right
			|| r2.right < r1.left
			|| r2.top > r1.bottom
			|| r2.bottom < r1.top);
	}
}

#endif