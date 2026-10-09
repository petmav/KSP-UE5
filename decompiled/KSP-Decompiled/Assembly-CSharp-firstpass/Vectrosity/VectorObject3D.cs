using UnityEngine;

namespace Vectrosity
{
	public class VectorObject3D : MonoBehaviour, IVectorObject
	{
		private bool deferredFirstUpdate;

		private bool m_updateVerts = true;

		private bool m_updateUVs = true;

		private bool m_updateColors = true;

		private bool m_updateNormals;

		private bool m_updateTangents;

		private bool m_updateTris = true;

		private Mesh m_mesh;

		private VectorLine m_vectorLine;

		private Material m_material;

		private bool m_useCustomMaterial;

		public bool updateImmediate;

		private bool ImmediateUpdate
		{
			get
			{
				if (updateImmediate)
				{
					return deferredFirstUpdate;
				}
				return false;
			}
		}

		public bool UpdateImmediate
		{
			get
			{
				return updateImmediate;
			}
			set
			{
				updateImmediate = value;
			}
		}

		public void SetVectorLine(VectorLine vectorLine, Texture tex, Material mat, bool useCustomMaterial)
		{
			base.gameObject.AddComponent<MeshRenderer>();
			base.gameObject.AddComponent<MeshFilter>();
			m_vectorLine = vectorLine;
			m_material = mat;
			m_material.mainTexture = tex;
			GetComponent<MeshRenderer>().sharedMaterial = m_material;
			m_useCustomMaterial = useCustomMaterial;
			SetupMesh();
		}

		public void Destroy()
		{
			Object.Destroy(m_mesh);
			if (!m_useCustomMaterial)
			{
				Object.Destroy(m_material);
			}
		}

		public void Enable(bool enable)
		{
			if (!(this == null))
			{
				GetComponent<MeshRenderer>().enabled = enable;
			}
		}

		public void SetTexture(Texture tex)
		{
			GetComponent<MeshRenderer>().sharedMaterial.mainTexture = tex;
		}

		public void SetMaterial(Material mat)
		{
			m_material = mat;
			m_useCustomMaterial = true;
			GetComponent<MeshRenderer>().sharedMaterial = mat;
			if (mat != null)
			{
				GetComponent<MeshRenderer>().sharedMaterial.mainTexture = m_vectorLine.texture;
			}
		}

		private void SetupMesh()
		{
			m_mesh = new Mesh();
			m_mesh.name = m_vectorLine.name;
			m_mesh.hideFlags = HideFlags.HideAndDontSave;
			GetComponent<MeshFilter>().mesh = m_mesh;
		}

		private void LateUpdate()
		{
			if (!ImmediateUpdate)
			{
				if (m_updateVerts)
				{
					SetVerts();
					m_updateVerts = false;
				}
				if (m_updateUVs)
				{
					SetUVs();
					m_updateUVs = false;
				}
				if (m_updateColors)
				{
					SetColors();
					m_updateColors = false;
				}
				if (m_updateTris)
				{
					SetTris();
					m_updateTris = false;
				}
				if (m_updateNormals)
				{
					SetNormals();
					m_updateNormals = false;
				}
				if (m_updateTangents)
				{
					SetTangents();
					m_updateTangents = false;
				}
				deferredFirstUpdate = true;
			}
		}

		private void SetVerts()
		{
			m_mesh.vertices = m_vectorLine.lineVertices;
			m_mesh.RecalculateBounds();
		}

		private void SetUVs()
		{
			if (m_vectorLine.lineUVs.Length == m_mesh.vertexCount)
			{
				m_mesh.uv = m_vectorLine.lineUVs;
			}
		}

		private void SetColors()
		{
			if (m_vectorLine.lineColors.Length == m_mesh.vertexCount)
			{
				m_mesh.colors32 = m_vectorLine.lineColors;
			}
		}

		private void SetNormals()
		{
			m_mesh.RecalculateNormals();
		}

		private void SetTangents()
		{
			m_mesh.tangents = m_vectorLine.CalculateTangents(m_mesh.normals);
		}

		private void SetTris()
		{
			m_mesh.SetTriangles(m_vectorLine.lineTriangles, 0);
		}

		public void SetName(string name)
		{
			if (!(m_mesh == null))
			{
				m_mesh.name = name;
			}
		}

		public void UpdateVerts()
		{
			if (ImmediateUpdate)
			{
				SetVerts();
			}
			else
			{
				m_updateVerts = true;
			}
		}

		public void UpdateUVs()
		{
			if (ImmediateUpdate)
			{
				SetUVs();
			}
			else
			{
				m_updateUVs = true;
			}
		}

		public void UpdateColors()
		{
			if (ImmediateUpdate)
			{
				SetColors();
			}
			else
			{
				m_updateColors = true;
			}
		}

		public void UpdateNormals()
		{
			if (ImmediateUpdate)
			{
				SetNormals();
			}
			else
			{
				m_updateNormals = true;
			}
		}

		public void UpdateTangents()
		{
			if (ImmediateUpdate)
			{
				SetTangents();
			}
			else
			{
				m_updateTangents = true;
			}
		}

		public void UpdateTris()
		{
			if (ImmediateUpdate)
			{
				SetTris();
			}
			else
			{
				m_updateTris = true;
			}
		}

		public void UpdateMeshAttributes()
		{
			m_mesh.Clear();
			m_updateVerts = true;
			m_updateUVs = true;
			m_updateColors = true;
			m_updateTris = true;
		}

		public void ClearMesh()
		{
			if (!(m_mesh == null))
			{
				m_mesh.Clear();
			}
		}

		public int VertexCount()
		{
			return m_mesh.vertexCount;
		}
	}
}
