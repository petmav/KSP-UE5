using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine.U2D;

[MovedFrom("UnityEngine.Experimental.U2D")]
public struct SpriteShapeSegment
{
	private int m_GeomIndex;

	private int m_IndexCount;

	private int m_VertexCount;

	private int m_SpriteIndex;

	public int geomIndex
	{
		get
		{
			return m_GeomIndex;
		}
		set
		{
			m_GeomIndex = value;
		}
	}

	public int indexCount
	{
		get
		{
			return m_IndexCount;
		}
		set
		{
			m_IndexCount = value;
		}
	}

	public int vertexCount
	{
		get
		{
			return m_VertexCount;
		}
		set
		{
			m_VertexCount = value;
		}
	}

	public int spriteIndex
	{
		get
		{
			return m_SpriteIndex;
		}
		set
		{
			m_SpriteIndex = value;
		}
	}
}
