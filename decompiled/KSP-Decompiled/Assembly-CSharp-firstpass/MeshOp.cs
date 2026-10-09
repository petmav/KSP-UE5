using UnityEngine;

public class MeshOp : MonoBehaviour
{
	public static void AddReducedMeshColliders(GameObject[] gs, string method)
	{
		if (!(method == "progmesh"))
		{
			return;
		}
		for (int i = 0; i < gs.Length; i++)
		{
			if (gs[i].GetComponent<MeshFilter>().mesh != null)
			{
				Mesh mesh = gs[i].GetComponent<MeshFilter>().mesh;
				string text = "_CM_" + mesh.name;
				if (mesh.vertices.Length > 255)
				{
					ProgMesh progMesh = new ProgMesh();
					progMesh.CalculatePM(mesh);
					int num = Mathf.FloorToInt(255f + 4245f * ((float)progMesh.VerticesTotal / 65000f));
					if (num > progMesh.VerticesTotal)
					{
						num = progMesh.VerticesTotal;
					}
					progMesh.UpdateMesh(num);
					Mesh mesh2 = new Mesh();
					mesh2.vertices = progMesh.finalvertices;
					mesh2.normals = progMesh.finalnormals;
					mesh2.triangles = progMesh.finaltriangles;
					gs[i].AddComponent<MeshCollider>();
					gs[i].GetComponent<MeshCollider>().sharedMesh = mesh2;
					gs[i].GetComponent<MeshCollider>().sharedMesh.name = text;
				}
				else
				{
					gs[i].AddComponent<MeshCollider>();
					gs[i].GetComponent<MeshCollider>().sharedMesh = gs[i].GetComponent<MeshFilter>().mesh;
				}
			}
			else
			{
				Debug.Log("Transform Only :" + gs[i].name);
			}
		}
	}
}
