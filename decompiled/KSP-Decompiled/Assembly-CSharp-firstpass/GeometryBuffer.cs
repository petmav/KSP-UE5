using System.Collections.Generic;
using UnityEngine;

public class GeometryBuffer
{
	public class ObjectData
	{
		public string name;

		public List<GroupData> groups;

		public List<FaceIndices> allFaces;

		public ObjectData()
		{
			groups = new List<GroupData>();
			allFaces = new List<FaceIndices>();
		}
	}

	public class GroupData
	{
		public string name;

		public string materialName;

		public List<FaceIndices> faces;

		public bool isEmpty => faces.Count == 0;

		public GroupData()
		{
			faces = new List<FaceIndices>();
		}
	}

	public List<ObjectData> objects;

	public List<Vector3> vertices;

	public List<Vector2> uvs;

	public List<Vector3> normals;

	private bool reorderfaces;

	private ObjectData current;

	private GroupData curgr;

	public int numObjects => objects.Count;

	public bool isEmpty => vertices.Count == 0;

	public bool hasUVs => uvs.Count > 0;

	public bool hasNormals => normals.Count > 0;

	public GeometryBuffer()
	{
		objects = new List<ObjectData>();
		ObjectData objectData = new ObjectData
		{
			name = "_spc_default"
		};
		objects.Add(objectData);
		current = objectData;
		GroupData item = new GroupData
		{
			name = "_spc_default",
			materialName = "_spc_default"
		};
		objectData.groups.Add(item);
		curgr = item;
		vertices = new List<Vector3>();
		uvs = new List<Vector2>();
		normals = new List<Vector3>();
	}

	public void PushObject(string name)
	{
		if (curgr.isEmpty)
		{
			objects.Remove(current);
		}
		ObjectData objectData = new ObjectData();
		objectData.name = name;
		objects.Add(objectData);
		GroupData groupData = new GroupData();
		groupData.name = "_spc_default";
		objectData.groups.Add(groupData);
		curgr = groupData;
		current = objectData;
	}

	public void PushGroup(string name)
	{
		if (curgr.isEmpty)
		{
			current.groups.Remove(curgr);
		}
		GroupData groupData = new GroupData();
		groupData.name = name;
		groupData.materialName = "_spc_default";
		current.groups.Add(groupData);
		curgr = groupData;
	}

	public void PushMaterialGroup(string name)
	{
		bool flag = false;
		foreach (GroupData group in current.groups)
		{
			if (group.name == name)
			{
				curgr = group;
				curgr.materialName = name;
				flag = true;
				reorderfaces = true;
			}
		}
		if (!flag)
		{
			if (curgr.isEmpty)
			{
				current.groups.Remove(curgr);
			}
			GroupData groupData2 = new GroupData();
			groupData2.name = name;
			groupData2.materialName = name;
			current.groups.Add(groupData2);
			curgr = groupData2;
		}
	}

	public string[] ReturnMaterialNames()
	{
		int num = 0;
		foreach (ObjectData @object in objects)
		{
			num += @object.groups.Count;
		}
		string[] array = new string[num];
		num = 0;
		foreach (ObjectData object2 in objects)
		{
			foreach (GroupData group in object2.groups)
			{
				array[num] = group.materialName;
				num++;
			}
		}
		return array;
	}

	public void PushVertex(Vector3 v)
	{
		vertices.Add(v);
	}

	public void PushUV(Vector2 v)
	{
		uvs.Add(v);
	}

	public void PushNormal(Vector3 v)
	{
		normals.Add(v);
	}

	public void PushFace(FaceIndices f)
	{
		curgr.faces.Add(f);
		current.allFaces.Add(f);
	}

	public bool ResolveLimit()
	{
		List<ObjectData> list = new List<ObjectData>();
		bool flag = true;
		bool flag2 = false;
		foreach (ObjectData @object in objects)
		{
			if (@object.allFaces.Count > 65000)
			{
				foreach (GroupData group in @object.groups)
				{
					if (group.faces.Count > 65000)
					{
						flag = false;
						Debug.Log("Group with more than 65000 Verts");
					}
				}
				if (!flag)
				{
					break;
				}
				Debug.Log("Trying to Resolve");
				foreach (GroupData group2 in @object.groups)
				{
					ObjectData objectData2 = new ObjectData();
					objectData2.name = group2.name;
					objectData2.groups.Add(group2);
					objectData2.allFaces = group2.faces;
					list.Add(objectData2);
				}
			}
			else
			{
				list.Add(@object);
			}
		}
		if (flag)
		{
			objects = list;
			return true;
		}
		return false;
	}

	public bool Check(bool AutoResolveVLimit, bool DebugOut)
	{
		bool result = true;
		if (DebugOut)
		{
			Debug.Log("OBJ has " + objects.Count + " object(s)");
			Debug.Log("OBJ has " + vertices.Count + " vertice(s)");
			Debug.Log("OBJ has " + uvs.Count + " uv(s)");
			Debug.Log("OBJ has " + normals.Count + " normal(s)");
		}
		foreach (ObjectData @object in objects)
		{
			if (DebugOut)
			{
				Debug.Log(@object.name + " has " + @object.groups.Count + " group(s)" + @object.allFaces.Count + "faces");
			}
			if (@object.allFaces.Count >= 64000)
			{
				result = false;
			}
			foreach (GroupData group in @object.groups)
			{
				if (DebugOut)
				{
					Debug.Log(@object.name + "/" + group.name + " has " + group.faces.Count + " faces(s)");
				}
			}
		}
		return result;
	}

	public void PopulateMeshes(GameObject[] gs, Dictionary<string, Material> mats)
	{
		if (gs.Length != numObjects)
		{
			return;
		}
		for (int i = 0; i < gs.Length; i++)
		{
			ObjectData objectData = objects[i];
			if (reorderfaces)
			{
				objectData.allFaces = new List<FaceIndices>();
				foreach (GroupData group in objectData.groups)
				{
					foreach (FaceIndices face in group.faces)
					{
						objectData.allFaces.Add(face);
					}
				}
			}
			if (objectData.name != "_spc_default" && gs.Length != 1)
			{
				gs[i].name = objectData.name;
			}
			if (objectData.name != "_spc_default" && gs.Length == 1 && gs[0].name == "_spc_rename_")
			{
				gs[i].name = objectData.name;
			}
			Vector3[] array = new Vector3[objectData.allFaces.Count];
			Vector2[] array2 = new Vector2[objectData.allFaces.Count];
			Vector3[] array3 = new Vector3[objectData.allFaces.Count];
			int num = 0;
			foreach (FaceIndices allFace in objectData.allFaces)
			{
				array[num] = vertices[allFace.vi];
				if (hasUVs)
				{
					array2[num] = uvs[allFace.vu];
				}
				if (hasNormals)
				{
					array3[num] = normals[allFace.vn];
				}
				num++;
			}
			Mesh mesh = (gs[i].GetComponent(typeof(MeshFilter)) as MeshFilter).mesh;
			mesh.vertices = array;
			if (hasUVs)
			{
				mesh.uv = array2;
			}
			if (hasNormals)
			{
				mesh.normals = array3;
			}
			bool flag = false;
			if (objectData.groups.Count == 1)
			{
				GroupData groupData = objectData.groups[0];
				int[] array4 = new int[groupData.faces.Count];
				for (int j = 0; j < array4.Length; j++)
				{
					array4[j] = j;
				}
				mesh.triangles = array4;
				if (mats.ContainsKey(groupData.materialName))
				{
					gs[i].GetComponent<Renderer>().material = mats[groupData.materialName];
					gs[i].GetComponent<Renderer>().material.name = groupData.materialName;
					if (mats[groupData.materialName].shader.name.Contains("Bumped "))
					{
						flag = true;
					}
				}
			}
			else
			{
				int count = objectData.groups.Count;
				Material[] array5 = new Material[count];
				mesh.subMeshCount = count;
				int num2 = 0;
				for (int k = 0; k < count; k++)
				{
					if (mats.ContainsKey(objectData.groups[k].materialName))
					{
						array5[k] = mats[objectData.groups[k].materialName];
						array5[k].name = objectData.groups[k].materialName;
						if (array5[k].shader.name.Contains("Bumped "))
						{
							flag = true;
						}
					}
					int[] array6 = new int[objectData.groups[k].faces.Count];
					int num3 = objectData.groups[k].faces.Count + num2;
					int num4 = 0;
					while (num2 < num3)
					{
						array6[num4] = num2;
						num2++;
						num4++;
					}
					mesh.SetTriangles(array6, k);
				}
				gs[i].GetComponent<Renderer>().materials = array5;
			}
			if (!hasNormals)
			{
				mesh.RecalculateNormals();
			}
			if (flag)
			{
				TangentSolver(mesh);
			}
		}
	}

	public static void TangentSolver(Mesh mesh)
	{
		Vector3[] array = new Vector3[mesh.vertices.Length];
		Vector3[] array2 = new Vector3[mesh.vertices.Length];
		Vector4[] array3 = new Vector4[mesh.vertices.Length];
		for (int i = 0; i < mesh.triangles.Length; i += 3)
		{
			long num = mesh.triangles[i];
			long num2 = mesh.triangles[i + 1];
			long num3 = mesh.triangles[i + 2];
			Vector3 vector = mesh.vertices[num];
			Vector3 vector2 = mesh.vertices[num2];
			Vector3 vector3 = mesh.vertices[num3];
			Vector2 vector4 = mesh.uv[num];
			Vector2 vector5 = mesh.uv[num2];
			Vector2 vector6 = mesh.uv[num3];
			float num4 = vector2.x - vector.x;
			float num5 = vector3.x - vector.x;
			float num6 = vector2.y - vector.y;
			float num7 = vector3.y - vector.y;
			float num8 = vector2.z - vector.z;
			float num9 = vector3.z - vector.z;
			float num10 = vector5.x - vector4.x;
			float num11 = vector6.x - vector4.x;
			float num12 = vector5.y - vector4.y;
			float num13 = vector6.y - vector4.y;
			float num14 = 1f / (num10 * num13 - num11 * num12);
			Vector3 vector7 = new Vector3((num13 * num4 - num12 * num5) * num14, (num13 * num6 - num12 * num7) * num14, (num13 * num8 - num12 * num9) * num14);
			Vector3 vector8 = new Vector3((num10 * num5 - num11 * num4) * num14, (num10 * num7 - num11 * num6) * num14, (num10 * num9 - num11 * num8) * num14);
			array2[num] += vector7;
			array2[num2] += vector7;
			array2[num3] += vector7;
			array[num] += vector8;
			array[num2] += vector8;
			array[num3] += vector8;
		}
		for (int j = 0; j < mesh.vertices.Length; j++)
		{
			Vector3 normal = mesh.normals[j];
			Vector3 tangent = array2[j];
			Vector3.OrthoNormalize(ref normal, ref tangent);
			array3[j].x = tangent.x;
			array3[j].y = tangent.y;
			array3[j].z = tangent.z;
			array3[j].w = ((Vector3.Dot(Vector3.Cross(normal, tangent), array[j]) < 0f) ? (-1f) : 1f);
		}
		mesh.tangents = array3;
	}
}
