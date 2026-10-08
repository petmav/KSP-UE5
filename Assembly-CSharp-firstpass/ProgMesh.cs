using System.Collections.Generic;
using UnityEngine;

public class ProgMesh
{
	private class Triangle
	{
		public Vertex[] vertex;

		public Vector3 normal;

		public bool deleted;

		public Triangle(Vertex _v0, Vertex _v1, Vertex _v2)
		{
			vertex = new Vertex[3];
			vertex[0] = _v0;
			vertex[1] = _v1;
			vertex[2] = _v2;
			ComputeNormal();
			for (int i = 0; i < 3; i++)
			{
				vertex[i].face.Add(this);
				for (int j = 0; j < 3; j++)
				{
					if (i == j)
					{
						continue;
					}
					bool flag = false;
					for (int k = 0; k < vertex[i].neighbour.Count; k++)
					{
						if (vertex[i].neighbour[k].id == vertex[j].id)
						{
							flag = true;
						}
					}
					if (!flag)
					{
						vertex[i].neighbour.Add(vertex[j]);
					}
				}
			}
		}

		public void RemoveTriangle()
		{
			for (int i = 0; i < 3; i++)
			{
				if (vertex[i] != null)
				{
					vertex[i].face.Remove(this);
				}
			}
			for (int i = 0; i < 3; i++)
			{
				int num = (i + 1) % 3;
				if (vertex[i] != null && vertex[num] != null)
				{
					vertex[i].RemoveIfNonNeighbour(vertex[num]);
					vertex[num].RemoveIfNonNeighbour(vertex[i]);
				}
			}
		}

		public bool HasVertex(Vertex v)
		{
			if (v != vertex[0] && v != vertex[1])
			{
				return v == vertex[2];
			}
			return true;
		}

		public void ComputeNormal()
		{
			Vector3 position = vertex[1].position;
			normal = Vector3.Cross(position - vertex[0].position, vertex[2].position - position);
			if (normal.magnitude != 0f)
			{
				normal.Normalize();
			}
		}

		public void ReplaceVertex(Vertex vold, Vertex vnew)
		{
			if (vold == vertex[0])
			{
				vertex[0] = vnew;
			}
			else if (vold == vertex[1])
			{
				vertex[1] = vnew;
			}
			else
			{
				vertex[2] = vnew;
			}
			vold.face.Remove(this);
			vnew.face.Add(this);
			for (int i = 0; i < 3; i++)
			{
				vold.RemoveIfNonNeighbour(vertex[i]);
				vertex[i].RemoveIfNonNeighbour(vold);
			}
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					if (i != j && !vertex[i].neighbour.Contains(vertex[j]))
					{
						vertex[i].neighbour.Add(vertex[j]);
					}
				}
			}
			ComputeNormal();
		}
	}

	private class Vertex
	{
		public Vector3 position;

		public List<Triangle> face;

		public List<Vertex> neighbour;

		public int id;

		public float cost;

		public Vertex collapse;

		private bool? _isBorder;

		public bool IsBorder
		{
			get
			{
				if (!_isBorder.HasValue)
				{
					_isBorder = IsABorderVertex(this);
				}
				return _isBorder.Value;
			}
		}

		public Vertex(Vector3 vec, int _id)
		{
			position = vec;
			id = _id;
			neighbour = new List<Vertex>();
			face = new List<Triangle>();
		}

		public void RemoveVertex()
		{
			while (neighbour.Count > 0)
			{
				neighbour[0].neighbour.Remove(this);
				neighbour.Remove(neighbour[0]);
			}
		}

		private static bool IsABorderVertex(Vertex v)
		{
			for (int i = 0; i < v.neighbour.Count; i++)
			{
				int num = 0;
				for (int j = 0; j < v.face.Count; j++)
				{
					if (v.face[j].HasVertex(v.neighbour[i]))
					{
						num++;
					}
				}
				if (num == 1)
				{
					return true;
				}
			}
			return false;
		}

		public void RemoveIfNonNeighbour(Vertex v)
		{
			if (!neighbour.Contains(v))
			{
				return;
			}
			for (int i = 0; i < face.Count; i++)
			{
				if (face[i].HasVertex(v))
				{
					return;
				}
			}
			neighbour.Remove(v);
		}
	}

	public int render_num;

	public bool Calculated;

	public bool Calculating;

	public int VerticesTotal;

	private Vector3[] origvertices;

	private Vector3[] orignormals;

	private int[] origtriangles;

	private Vector3[] sharedvertices;

	private Vector3[] sharednormals;

	private int[] sharedtriangles;

	private static List<Triangle> triangles = new List<Triangle>();

	private static List<Vertex> vertices = new List<Vertex>();

	private int[] map;

	private int[] order;

	public Vector3[] finalvertices;

	public Vector3[] finalnormals;

	public int[] finaltriangles;

	private static bool deleted(Triangle t)
	{
		if (t.deleted)
		{
			return true;
		}
		return false;
	}

	private float ComputeEdgeCollapseCost(Vertex u, Vertex v)
	{
		float magnitude = (v.position - u.position).magnitude;
		float num = 0f;
		List<Triangle> list = new List<Triangle>();
		for (int i = 0; i < u.face.Count; i++)
		{
			if (u.face[i].HasVertex(v))
			{
				list.Add(u.face[i]);
			}
		}
		for (int i = 0; i < u.face.Count; i++)
		{
			float num2 = 1f;
			for (int j = 0; j < list.Count; j++)
			{
				float num3 = Vector3.Dot(u.face[i].normal, list[j].normal);
				num2 = Mathf.Min(num2, (1f - num3) / 2f);
			}
			num = Mathf.Max(num, num2);
		}
		if (u.IsBorder && list.Count > 1)
		{
			num = 1f;
		}
		return magnitude * num;
	}

	private void ComputeEdgeCostAtVertex(Vertex v)
	{
		if (v.neighbour.Count == 0)
		{
			v.collapse = null;
			v.cost = -0.01f;
			return;
		}
		v.cost = 1000000f;
		v.collapse = null;
		for (int i = 0; i < v.neighbour.Count; i++)
		{
			float num = ComputeEdgeCollapseCost(v, v.neighbour[i]);
			if (num < v.cost)
			{
				v.collapse = v.neighbour[i];
				v.cost = num;
			}
		}
	}

	private void ComputeAllEdgeCollapseCosts()
	{
		for (int i = 0; i < vertices.Count; i++)
		{
			ComputeEdgeCostAtVertex(vertices[i]);
		}
	}

	private void Collapse(Vertex u, Vertex v)
	{
		if (v == null)
		{
			u.RemoveVertex();
			vertices.Remove(u);
			return;
		}
		List<Vertex> list = new List<Vertex>();
		for (int i = 0; i < u.neighbour.Count; i++)
		{
			list.Add(u.neighbour[i]);
		}
		for (int i = u.face.Count - 1; i >= 0; i--)
		{
			if (u.face[i].HasVertex(v))
			{
				u.face[i].deleted = true;
			}
		}
		foreach (Triangle item in u.face.FindAll(deleted))
		{
			item.RemoveTriangle();
		}
		for (int i = u.face.Count - 1; i >= 0; i--)
		{
			u.face[i].ReplaceVertex(u, v);
		}
		u.RemoveVertex();
		vertices.Remove(u);
		for (int i = 0; i < list.Count; i++)
		{
			ComputeEdgeCostAtVertex(list[i]);
		}
	}

	private Vertex MinimumCostEdge()
	{
		Vertex vertex = vertices[0];
		for (int i = 0; i < vertices.Count; i++)
		{
			if (vertices[i].cost < vertex.cost)
			{
				vertex = vertices[i];
			}
		}
		return vertex;
	}

	public void CalculatePM(Mesh mesh)
	{
		Calculated = false;
		origvertices = mesh.vertices;
		orignormals = mesh.normals;
		origtriangles = mesh.triangles;
		if (0 == 0)
		{
			int num = -1;
			int[] array = mesh.triangles;
			List<Vector3> list = new List<Vector3>();
			List<Vector3> list2 = new List<Vector3>();
			for (int i = 0; i < array.Length; i++)
			{
				int num2 = array[i];
				Vector3 vector = origvertices[num2];
				Vector3 item = orignormals[num2];
				int count = list.Count;
				num = -1;
				for (int j = 0; j < count; j++)
				{
					if (list[j] == vector)
					{
						num = j;
						break;
					}
				}
				if (num != -1)
				{
					array[i] = num;
					continue;
				}
				array[i] = count;
				list.Add(vector);
				list2.Add(item);
			}
			sharedtriangles = array;
			sharedvertices = list.ToArray();
			sharednormals = list2.ToArray();
		}
		else
		{
			sharedtriangles = origtriangles;
			sharedvertices = origvertices;
			sharednormals = orignormals;
		}
		int num3 = sharedvertices.Length;
		int num4 = sharedtriangles.Length;
		map = new int[sharedvertices.Length];
		order = new int[sharedvertices.Length];
		vertices = null;
		vertices = new List<Vertex>();
		for (int i = 0; i < num3; i++)
		{
			Vertex item2 = new Vertex(sharedvertices[i], i);
			vertices.Add(item2);
		}
		triangles = null;
		triangles = new List<Triangle>();
		for (int i = 0; i < num4; i += 3)
		{
			Triangle item3 = new Triangle(vertices[sharedtriangles[i]], vertices[sharedtriangles[i + 1]], vertices[sharedtriangles[i + 2]]);
			triangles.Add(item3);
		}
		ComputeAllEdgeCollapseCosts();
		while (vertices.Count > 0)
		{
			Vertex vertex = MinimumCostEdge();
			order[vertex.id] = vertices.Count - 1;
			map[vertices.Count - 1] = ((vertex.collapse != null) ? vertex.collapse.id : (-1));
			Collapse(vertex, vertex.collapse);
		}
		for (int i = 0; i < map.Length; i++)
		{
			map[i] = ((map[i] != -1) ? order[map[i]] : 0);
		}
		PermuteVertices();
		Calculated = true;
		VerticesTotal = sharedvertices.Length;
	}

	private int Map(int a, int mx)
	{
		if (mx <= 0)
		{
			return 0;
		}
		while (a >= mx)
		{
			a = map[a];
		}
		return a;
	}

	private void PermuteVertices()
	{
		List<Vector3> list = new List<Vector3>();
		List<Vector3> list2 = new List<Vector3>();
		if (order.Length == sharedvertices.Length)
		{
			for (int i = 0; i < sharedvertices.Length; i++)
			{
				list.Add(sharedvertices[i]);
				list2.Add(sharednormals[i]);
			}
			for (int i = 0; i < sharedvertices.Length; i++)
			{
				sharedvertices[order[i]] = list[i];
				sharednormals[order[i]] = list2[i];
			}
			for (int i = 0; i < origtriangles.Length; i += 3)
			{
				sharedtriangles[i] = order[sharedtriangles[i]];
				sharedtriangles[i + 1] = order[sharedtriangles[i + 1]];
				sharedtriangles[i + 2] = order[sharedtriangles[i + 2]];
			}
		}
	}

	public void UpdateMesh(float ratio)
	{
		UpdateMesh(Mathf.RoundToInt((float)sharedvertices.Length * (ratio * 0.01f)));
	}

	public void UpdateMesh(int numtorender)
	{
		Calculating = true;
		render_num = numtorender;
		if (map.Length != 0)
		{
			List<int> list = new List<int>();
			for (int i = 0; i < sharedtriangles.Length; i += 3)
			{
				int num = Map(sharedtriangles[i], render_num);
				int num2 = Map(sharedtriangles[i + 1], render_num);
				int num3 = Map(sharedtriangles[i + 2], render_num);
				if (num != num2 && num2 != num3 && num3 != num)
				{
					list.Add(num);
					list.Add(num2);
					list.Add(num3);
				}
			}
			finaltriangles = list.ToArray();
			finalvertices = null;
			finalvertices = new Vector3[render_num];
			finalnormals = null;
			finalnormals = new Vector3[render_num];
			for (int j = 0; j < finaltriangles.Length; j++)
			{
				finalnormals[finaltriangles[j]] = sharednormals[finaltriangles[j]];
				finalvertices[finaltriangles[j]] = sharedvertices[finaltriangles[j]];
			}
		}
		Calculating = false;
	}
}
