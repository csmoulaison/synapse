#define FLOAT_EPSILON 0.00001

struct v2 
{
	union 
	{
    	float data[2];
    	struct 
    	{
        	float x;
        	float y;
    	};
	};
};

struct v3
{
	union 
	{
    	float data[3];
    	struct 
    	{
        	float x;
        	float y;
        	float z;
    	};
    	struct
    	{
        	float r;
        	float g;
        	float b;
    	};
	};
};

struct v4
{
	union 
	{
    	float data[4];
    	struct 
    	{
        	float x;
        	float y;
        	float z;
        	float w;
    	};
    	struct
    	{
        	float r;
        	float g;
        	float b;
        	float a;
    	};
	};
};

struct v3_line 
{
    struct v3 a;
    struct v3 b;
};

struct v3 v3_new(float x, float y, float z)
{
    return (struct v3){{{x, y, z}}};
}

struct v3 v3_zero()
{
    return v3_new(0.0f, 0.0f, 0.0f);
}

struct v3 v3_add(struct v3 a, struct v3 b)
{
    return v3_new(
    	a.data[0] + b.data[0],
    	a.data[1] + b.data[1],
    	a.data[2] + b.data[2]);
}

struct v3 v3_sub(struct v3 a, struct v3 b)
{
    return v3_new(
    	a.data[0] - b.data[0],
    	a.data[1] - b.data[1],
    	a.data[2] - b.data[2]);
}

struct v3 v3_scale(struct v3 v, float s)
{
    return v3_new(
        v.data[0] * s,
        v.data[1] * s,
        v.data[2] * s);
}

float v3_magnitude(struct v3 v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

struct v3 v3_normalize(struct v3 v)
{
	float magnitude = v3_magnitude(v);

	// TODO - is this reasonable?
	if(magnitude < FLOAT_EPSILON) {
    	return v3_zero();
	}

	return v3_new(
    	v.x / magnitude,
    	v.y / magnitude,
    	v.z / magnitude);
}

struct v3 v3_cross(struct v3 a, struct v3 b)
{
    return v3_new(
        a.data[1] * b.data[2] - a.data[2] * b.data[1],
        a.data[2] * b.data[0] - a.data[0] * b.data[2],
        a.data[0] * b.data[1] - a.data[1] * b.data[0]);
}

// TODO - our own m4 struct
void m4_lookat(struct v3 eye, struct v3 center, struct v3 up, mat4 dst)
{
	glm_lookat(eye.data, center.data, up.data, dst);
}

float radians(float degrees)
{
	return glm_rad(degrees);
}
