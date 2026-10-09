using System.IO;
using UnityEngine;

public class ProjectModelLoad : MonoBehaviour
{
	public static Mesh ReadMesh(byte[] bytes)
	{
		if (bytes.Length != 0 && bytes.Length < 5)
		{
			MonoBehaviour.print("Invalid mesh file!");
			return null;
		}
		BinaryReader binaryReader = new BinaryReader(new MemoryStream(bytes));
		ushort num = binaryReader.ReadUInt16();
		ushort num2 = binaryReader.ReadUInt16();
		byte b = binaryReader.ReadByte();
		if (num < 0 || num > 64000)
		{
			MonoBehaviour.print("Invalid vertex count in the mesh data!");
			return null;
		}
		if (num2 < 0 || num2 > 64000)
		{
			MonoBehaviour.print("Invalid triangle count in the mesh data!");
			return null;
		}
		if (b < 1 || (b & 1) == 0 || b > 15)
		{
			MonoBehaviour.print("Invalid vertex format in the mesh data!");
			return null;
		}
		Mesh mesh = new Mesh();
		Vector3[] array = new Vector3[num];
		ReadVector3Array16bit(array, binaryReader);
		mesh.vertices = array;
		if ((b & 2) == 2)
		{
			Vector3[] array2 = new Vector3[num];
			ReadVector3ArrayBytes(array2, binaryReader);
			mesh.normals = array2;
		}
		if ((b & 4) == 4)
		{
			Vector4[] array3 = new Vector4[num];
			ReadVector4ArrayBytes(array3, binaryReader);
			mesh.tangents = array3;
		}
		if ((b & 8) == 8)
		{
			Vector2[] array4 = new Vector2[num];
			ReadVector2Array16bit(array4, binaryReader);
			mesh.uv = array4;
		}
		int[] array5 = new int[num2 * 3];
		for (int i = 0; i < num2; i++)
		{
			array5[i * 3] = binaryReader.ReadUInt16();
			array5[i * 3 + 1] = binaryReader.ReadUInt16();
			array5[i * 3 + 2] = binaryReader.ReadUInt16();
		}
		mesh.triangles = array5;
		binaryReader.Close();
		return mesh;
	}

	private static void ReadVector3Array16bit(Vector3[] arr, BinaryReader buf)
	{
		int num = arr.Length;
		if (num != 0)
		{
			Vector3 vector = default(Vector3);
			Vector3 vector2 = default(Vector3);
			vector.x = buf.ReadSingle();
			vector2.x = buf.ReadSingle();
			vector.y = buf.ReadSingle();
			vector2.y = buf.ReadSingle();
			vector.z = buf.ReadSingle();
			vector2.z = buf.ReadSingle();
			for (int i = 0; i < num; i++)
			{
				ushort num2 = buf.ReadUInt16();
				ushort num3 = buf.ReadUInt16();
				ushort num4 = buf.ReadUInt16();
				float x = (float)(int)num2 / 65535f * (vector2.x - vector.x) + vector.x;
				float y = (float)(int)num3 / 65535f * (vector2.y - vector.y) + vector.y;
				float z = (float)(int)num4 / 65535f * (vector2.z - vector.z) + vector.z;
				arr[i] = new Vector3(x, y, z);
			}
		}
	}

	private static void ReadVector2Array16bit(Vector2[] arr, BinaryReader buf)
	{
		int num = arr.Length;
		if (num != 0)
		{
			Vector2 vector = default(Vector2);
			Vector2 vector2 = default(Vector2);
			vector.x = buf.ReadSingle();
			vector2.x = buf.ReadSingle();
			vector.y = buf.ReadSingle();
			vector2.y = buf.ReadSingle();
			for (int i = 0; i < num; i++)
			{
				ushort num2 = buf.ReadUInt16();
				ushort num3 = buf.ReadUInt16();
				float x = (float)(int)num2 / 65535f * (vector2.x - vector.x) + vector.x;
				float y = (float)(int)num3 / 65535f * (vector2.y - vector.y) + vector.y;
				arr[i] = new Vector2(x, y);
			}
		}
	}

	private static void ReadVector3ArrayBytes(Vector3[] arr, BinaryReader buf)
	{
		int num = arr.Length;
		for (int i = 0; i < num; i++)
		{
			byte num2 = buf.ReadByte();
			byte b = buf.ReadByte();
			byte b2 = buf.ReadByte();
			float x = ((float)(int)num2 - 128f) / 127f;
			float y = ((float)(int)b - 128f) / 127f;
			float z = ((float)(int)b2 - 128f) / 127f;
			arr[i] = new Vector3(x, y, z);
		}
	}

	private static void ReadVector4ArrayBytes(Vector4[] arr, BinaryReader buf)
	{
		int num = arr.Length;
		for (int i = 0; i < num; i++)
		{
			byte num2 = buf.ReadByte();
			byte b = buf.ReadByte();
			byte b2 = buf.ReadByte();
			byte b3 = buf.ReadByte();
			float x = ((float)(int)num2 - 128f) / 127f;
			float y = ((float)(int)b - 128f) / 127f;
			float z = ((float)(int)b2 - 128f) / 127f;
			float w = ((float)(int)b3 - 128f) / 127f;
			arr[i] = new Vector4(x, y, z, w);
		}
	}

	public static Mesh ReadMesh(WWW download)
	{
		if (download.error != null)
		{
			MonoBehaviour.print("Error downloading mesh: " + download.error);
			return null;
		}
		if (!download.isDone)
		{
			MonoBehaviour.print("Download must be finished already");
			return null;
		}
		return ReadMesh(download.bytes);
	}
}
