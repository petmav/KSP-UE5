using System.IO;
using UnityEngine;

public class ProjectModelSave : MonoBehaviour
{
	private static byte[] WriteMesh(Mesh mesh)
	{
		if (!mesh)
		{
			MonoBehaviour.print("No mesh given!");
			return null;
		}
		bool flag = false;
		Vector3[] vertices = mesh.vertices;
		Vector3[] normals = mesh.normals;
		Vector4[] tangents = mesh.tangents;
		Vector2[] uv = mesh.uv;
		int[] triangles = mesh.triangles;
		byte b = 1;
		if (normals.Length != 0)
		{
			b = (byte)(b | 2u);
		}
		if (tangents.Length != 0)
		{
			b = (byte)(b | 4u);
			flag = true;
			MonoBehaviour.print("saving tangents");
		}
		if (uv.Length != 0)
		{
			b = (byte)(b | 8u);
		}
		MemoryStream memoryStream = new MemoryStream();
		BinaryWriter binaryWriter = new BinaryWriter(memoryStream);
		ushort value = (ushort)vertices.Length;
		ushort value2 = (ushort)(triangles.Length / 3);
		binaryWriter.Write(value);
		binaryWriter.Write(value2);
		binaryWriter.Write(b);
		WriteVector3Array16bit(vertices, binaryWriter);
		WriteVector3ArrayBytes(normals, binaryWriter);
		if (flag)
		{
			WriteVector4ArrayBytes(tangents, binaryWriter);
		}
		WriteVector2Array16bit(uv, binaryWriter);
		int[] array = triangles;
		for (int i = 0; i < array.Length; i++)
		{
			ushort value3 = (ushort)array[i];
			binaryWriter.Write(value3);
		}
		binaryWriter.Close();
		memoryStream.Close();
		return memoryStream.ToArray();
	}

	private static void WriteVector3Array16bit(Vector3[] arr, BinaryWriter buf)
	{
		if (arr.Length != 0)
		{
			Bounds bounds = new Bounds(arr[0], new Vector3(0.001f, 0.001f, 0.001f));
			Vector3[] array = arr;
			foreach (Vector3 point in array)
			{
				bounds.Encapsulate(point);
			}
			Vector3 min = bounds.min;
			Vector3 max = bounds.max;
			buf.Write(min.x);
			buf.Write(max.x);
			buf.Write(min.y);
			buf.Write(max.y);
			buf.Write(min.z);
			buf.Write(max.z);
			array = arr;
			for (int i = 0; i < array.Length; i++)
			{
				Vector3 vector = array[i];
				float num = Mathf.Clamp((vector.x - min.x) / (max.x - min.x) * 65535f, 0f, 65535f);
				float num2 = Mathf.Clamp((vector.y - min.y) / (max.y - min.y) * 65535f, 0f, 65535f);
				float num3 = Mathf.Clamp((vector.z - min.z) / (max.z - min.z) * 65535f, 0f, 65535f);
				ushort value = (ushort)num;
				ushort value2 = (ushort)num2;
				ushort value3 = (ushort)num3;
				buf.Write(value);
				buf.Write(value2);
				buf.Write(value3);
			}
		}
	}

	private static void WriteVector2Array16bit(Vector2[] arr, BinaryWriter buf)
	{
		if (arr.Length != 0)
		{
			Vector2 vector = arr[0] - new Vector2(0.001f, 0.001f);
			Vector2 vector2 = arr[0] + new Vector2(0.001f, 0.001f);
			Vector2[] array = arr;
			for (int i = 0; i < array.Length; i++)
			{
				Vector2 vector3 = array[i];
				vector.x = Mathf.Min(vector.x, vector3.x);
				vector.y = Mathf.Min(vector.y, vector3.y);
				vector2.x = Mathf.Max(vector2.x, vector3.x);
				vector2.y = Mathf.Max(vector2.y, vector3.y);
			}
			buf.Write(vector.x);
			buf.Write(vector2.x);
			buf.Write(vector.y);
			buf.Write(vector2.y);
			array = arr;
			for (int i = 0; i < array.Length; i++)
			{
				Vector2 vector4 = array[i];
				float num = (vector4.x - vector.x) / (vector2.x - vector.x) * 65535f;
				float num2 = (vector4.y - vector.y) / (vector2.y - vector.y) * 65535f;
				ushort value = (ushort)num;
				ushort value2 = (ushort)num2;
				buf.Write(value);
				buf.Write(value2);
			}
		}
	}

	private static void WriteVector3ArrayBytes(Vector3[] arr, BinaryWriter buf)
	{
		for (int i = 0; i < arr.Length; i++)
		{
			Vector3 vector = arr[i];
			byte value = (byte)Mathf.Clamp(vector.x * 127f + 128f, 0f, 255f);
			byte value2 = (byte)Mathf.Clamp(vector.y * 127f + 128f, 0f, 255f);
			byte value3 = (byte)Mathf.Clamp(vector.z * 127f + 128f, 0f, 255f);
			buf.Write(value);
			buf.Write(value2);
			buf.Write(value3);
		}
	}

	private static void WriteVector4ArrayBytes(Vector4[] arr, BinaryWriter buf)
	{
		for (int i = 0; i < arr.Length; i++)
		{
			Vector4 vector = arr[i];
			byte value = (byte)Mathf.Clamp(vector.x * 127f + 128f, 0f, 255f);
			byte value2 = (byte)Mathf.Clamp(vector.y * 127f + 128f, 0f, 255f);
			byte value3 = (byte)Mathf.Clamp(vector.z * 127f + 128f, 0f, 255f);
			byte value4 = (byte)Mathf.Clamp(vector.w * 127f + 128f, 0f, 255f);
			buf.Write(value);
			buf.Write(value2);
			buf.Write(value3);
			buf.Write(value4);
		}
	}

	public static void WriteMesh(Mesh mesh, string FilePath)
	{
		byte[] array = WriteMesh(mesh);
		FileStream fileStream = new FileStream(FilePath, FileMode.Create);
		fileStream.Write(array, 0, array.Length);
		fileStream.Close();
	}

	public static void WriteTexture(Texture2D mTexture, string FilePath)
	{
		byte[] array = mTexture.EncodeToPNG();
		try
		{
			using (BinaryWriter binaryWriter = new BinaryWriter(File.Open(FilePath, FileMode.Create)))
			{
				binaryWriter.Write(array, 0, array.Length);
				binaryWriter.Close();
			}
		}
		catch
		{
			Debug.Log("Error writing texture");
		}
	}
}
