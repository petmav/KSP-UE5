using System.Collections.Generic;
using UnityEngine;

public class TriPoly
{
	private Vector3[] mInputPoints;

	private Vector3[] mPoints;

	private List<int> mIndices = new List<int>();

	private int[] Indices;

	private int[] V;

	public int[] Patch(Vector3[] Points)
	{
		mInputPoints = Points;
		Main();
		return Indices;
	}

	private bool Main()
	{
		bool result = false;
		if (mInputPoints != null && mInputPoints.Length > 2)
		{
			result = true;
			Vector3 vector = mInputPoints[0];
			Vector3 vector2 = mInputPoints[0];
			for (int i = 0; i < mInputPoints.Length; i++)
			{
				Vector3 vector3 = mInputPoints[i];
				if (vector3.x < vector.x)
				{
					vector.x = vector3.x;
				}
				if (vector3.y < vector.y)
				{
					vector.y = vector3.y;
				}
				if (vector3.z < vector.z)
				{
					vector.z = vector3.z;
				}
				if (vector3.x > vector2.x)
				{
					vector2.x = vector3.x;
				}
				if (vector3.y > vector2.y)
				{
					vector2.y = vector3.y;
				}
				if (vector3.z > vector2.z)
				{
					vector2.z = vector3.z;
				}
			}
			float num = vector2.x - vector.x;
			float num2 = vector2.y - vector.y;
			float num3 = vector2.z - vector.z;
			int index;
			int index2;
			int index3;
			if (num > num2 && num > num3)
			{
				index = 0;
				if (num2 > num3)
				{
					index2 = 1;
					index3 = 2;
				}
				else
				{
					index2 = 2;
					index3 = 1;
				}
			}
			else if (num2 > num && num2 > num3)
			{
				index = 1;
				if (num > num3)
				{
					index2 = 0;
					index3 = 2;
				}
				else
				{
					index2 = 2;
					index3 = 0;
				}
			}
			else
			{
				index = 2;
				if (num > num2)
				{
					index2 = 0;
					index3 = 1;
				}
				else
				{
					index2 = 1;
					index3 = 0;
				}
			}
			mPoints = new Vector3[mInputPoints.Length];
			Vector3 vector4 = default(Vector3);
			for (int j = 0; j < mInputPoints.Length; j++)
			{
				vector4 = mInputPoints[j];
				mPoints[j] = new Vector3(vector4[index], vector4[index2], vector4[index3]);
			}
			Triangulation();
			if (mIndices.Count > 2)
			{
				Indices = new int[mIndices.Count];
				for (int k = 0; k < mIndices.Count; k++)
				{
					Indices[k] = mIndices[k];
				}
			}
			else
			{
				result = false;
				Indices = new int[1];
				Indices[0] = -1;
			}
		}
		return result;
	}

	private void Triangulation()
	{
		int num = mPoints.Length;
		V = new int[num];
		bool flag = false;
		if (0f < _area())
		{
			for (int i = 0; i < num; i++)
			{
				V[i] = i;
			}
		}
		else
		{
			flag = true;
			for (int j = 0; j < num; j++)
			{
				V[j] = num - 1 - j;
			}
		}
		int num2 = num;
		int num3 = 2 * num2;
		int num4 = 0;
		int num5 = num2 - 1;
		while (num2 > 2 && 0 < num3--)
		{
			int num6 = num5;
			if (num2 <= num6)
			{
				num6 = 0;
			}
			num5 = num6 + 1;
			if (num2 <= num5)
			{
				num5 = 0;
			}
			int num7 = num5 + 1;
			if (num2 <= num7)
			{
				num7 = 0;
			}
			if (_snip(num6, num5, num7, num2))
			{
				int item = V[num6];
				int item2 = V[num5];
				int item3 = V[num7];
				if (flag)
				{
					mIndices.Add(item);
					mIndices.Add(item2);
					mIndices.Add(item3);
				}
				else
				{
					mIndices.Add(item3);
					mIndices.Add(item2);
					mIndices.Add(item);
				}
				num4++;
				int num8 = num5;
				for (int k = num5 + 1; k < num2; k++)
				{
					V[num8] = V[k];
					num8++;
				}
				num2--;
				num3 = 2 * num2;
			}
		}
	}

	private float _area()
	{
		int num = mPoints.Length;
		float num2 = 0f;
		int num3 = num - 1;
		int num4 = 0;
		while (num4 < num)
		{
			Vector3 vector = mPoints[num3];
			Vector3 vector2 = mPoints[num4];
			num2 += vector.x * vector2.y - vector2.x * vector.y;
			num3 = num4++;
		}
		return num2 * 0.5f;
	}

	private bool _snip(int u, int v, int w, int n)
	{
		Vector3 a = mPoints[V[u]];
		Vector3 b = mPoints[V[v]];
		Vector3 c = mPoints[V[w]];
		if (Mathf.Epsilon > (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x))
		{
			return false;
		}
		for (int i = 0; i < n; i++)
		{
			if (i != u && i != v && i != w)
			{
				Vector3 p = mPoints[V[i]];
				if (_insideTriangle(a, b, c, p))
				{
					return false;
				}
			}
		}
		return true;
	}

	private bool _insideTriangle(Vector3 A, Vector3 B, Vector3 C, Vector3 P)
	{
		float num = C.x - B.x;
		float num2 = C.y - B.y;
		float num3 = A.x - C.x;
		float num4 = A.y - C.y;
		float num5 = B.x - A.x;
		float num6 = B.y - A.y;
		float num7 = P.x - A.x;
		float num8 = P.y - A.y;
		float num9 = P.x - B.x;
		float num10 = P.y - B.y;
		float num11 = P.x - C.x;
		float num12 = P.y - C.y;
		float num13 = num * num10 - num2 * num9;
		float num14 = num5 * num8 - num6 * num7;
		float num15 = num3 * num12 - num4 * num11;
		if (num13 >= 0f && num15 >= 0f)
		{
			return num14 >= 0f;
		}
		return false;
	}
}
