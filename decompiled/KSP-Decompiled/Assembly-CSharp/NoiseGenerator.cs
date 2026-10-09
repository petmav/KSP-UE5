using System;
using UnityEngine;

public class NoiseGenerator
{
	private readonly int[] A = new int[3];

	private int[] T;

	private int i;

	private int j;

	private int k;

	private float onesixth = 1f / 6f;

	private float onethird = 1f / 3f;

	private float s;

	private float u;

	private float v;

	private float w;

	public NoiseGenerator()
	{
		if (T == null)
		{
			System.Random random = new System.Random();
			T = new int[8];
			for (int i = 0; i < 8; i++)
			{
				T[i] = random.Next();
			}
		}
	}

	public void SetSeed(int[] seed)
	{
		for (int i = 0; i < 8; i++)
		{
			T[i] = seed[i];
		}
	}

	public string GetSeed()
	{
		string text = "";
		for (int i = 0; i < 8; i++)
		{
			text += T[i];
			if (i < 7)
			{
				text += " ";
			}
		}
		return text;
	}

	public float coherentNoise(float x, float y, float z, int octaves = 1, int multiplier = 25, float amplitude = 0.5f, float lacunarity = 2f, float persistence = 0.9f)
	{
		Vector3 vector = new Vector3(x, y, z) / multiplier;
		float num = 0f;
		for (int i = 0; i < octaves; i++)
		{
			num += noise(vector.x, vector.y, vector.z) * amplitude;
			vector *= lacunarity;
			amplitude *= persistence;
		}
		return num;
	}

	public int getDensity(Vector3 loc)
	{
		float t = coherentNoise(loc.x, loc.y, loc.z);
		return (int)Mathf.Lerp(0f, 255f, t);
	}

	public float noise(float x, float y, float z)
	{
		s = (x + y + z) * onethird;
		i = fastfloor(x + s);
		j = fastfloor(y + s);
		k = fastfloor(z + s);
		s = (float)(i + j + k) * onesixth;
		u = x - (float)i + s;
		v = y - (float)j + s;
		w = z - (float)k + s;
		A[0] = 0;
		A[1] = 0;
		A[2] = 0;
		int num = ((!(u >= w)) ? ((v >= w) ? 1 : 2) : ((!(u >= v)) ? 1 : 0));
		int num2 = ((!(u < w)) ? ((v < w) ? 1 : 2) : ((!(u < v)) ? 1 : 0));
		return kay(num) + kay(3 - num - num2) + kay(num2) + kay(0);
	}

	private float kay(int a)
	{
		s = (float)(A[0] + A[1] + A[2]) * onesixth;
		float num = u - (float)A[0] + s;
		float num2 = v - (float)A[1] + s;
		float num3 = w - (float)A[2] + s;
		float num4 = 0.6f - num * num - num2 * num2 - num3 * num3;
		int num5 = shuffle(i + A[0], j + A[1], k + A[2]);
		A[a]++;
		if (num4 < 0f)
		{
			return 0f;
		}
		int num6 = (num5 >> 5) & 1;
		int num7 = (num5 >> 4) & 1;
		int num8 = (num5 >> 3) & 1;
		int num9 = (num5 >> 2) & 1;
		int num10 = num5 & 3;
		float num11;
		switch (num10)
		{
		default:
			num11 = num3;
			break;
		case 2:
			num11 = num2;
			break;
		case 1:
			num11 = num;
			break;
		}
		float num12 = num11;
		float num13;
		switch (num10)
		{
		default:
			num13 = num;
			break;
		case 2:
			num13 = num3;
			break;
		case 1:
			num13 = num2;
			break;
		}
		float num14 = num13;
		float num15;
		switch (num10)
		{
		default:
			num15 = num2;
			break;
		case 2:
			num15 = num;
			break;
		case 1:
			num15 = num3;
			break;
		}
		float num16 = num15;
		num12 = ((num6 == num8) ? (0f - num12) : num12);
		num14 = ((num6 == num7) ? (0f - num14) : num14);
		num16 = ((num6 != (num7 ^ num8)) ? (0f - num16) : num16);
		num4 *= num4;
		return 8f * num4 * num4 * (num12 + ((num10 == 0) ? (num14 + num16) : ((num9 == 0) ? num14 : num16)));
	}

	private int shuffle(int i, int j, int k)
	{
		return b(i, j, k, 0) + b(j, k, i, 1) + b(k, i, j, 2) + b(i, j, k, 3) + b(j, k, i, 4) + b(k, i, j, 5) + b(i, j, k, 6) + b(j, k, i, 7);
	}

	private int b(int i, int j, int k, int B)
	{
		return T[(b(i, B) << 2) | (b(j, B) << 1) | b(k, B)];
	}

	private int b(int N, int B)
	{
		return (N >> B) & 1;
	}

	private int fastfloor(float n)
	{
		if (!(n > 0f))
		{
			return (int)n - 1;
		}
		return (int)n;
	}
}
