using System.Runtime.CompilerServices;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Jobs;
using UnityEngine.Bindings;
using UnityEngine.Rendering;
using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine.U2D;

[MovedFrom("UnityEngine.Experimental.U2D")]
[NativeType(Header = "Modules/SpriteShape/Public/SpriteShapeRenderer.h")]
public class SpriteShapeRenderer : Renderer
{
	public Color color
	{
		get
		{
			get_color_Injected(out var ret);
			return ret;
		}
		set
		{
			set_color_Injected(ref value);
		}
	}

	public extern SpriteMaskInteraction maskInteraction
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public void Prepare(JobHandle handle, SpriteShapeParameters shapeParams, Sprite[] sprites)
	{
		Prepare_Injected(ref handle, ref shapeParams, sprites);
	}

	private unsafe NativeArray<T> GetNativeDataArray<T>(SpriteShapeDataType dataType) where T : struct
	{
		SpriteChannelInfo dataInfo = GetDataInfo(dataType);
		return NativeArrayUnsafeUtility.ConvertExistingDataToNativeArray<T>(dataInfo.buffer, dataInfo.count, Allocator.Invalid);
	}

	private unsafe NativeSlice<T> GetChannelDataArray<T>(SpriteShapeDataType dataType, VertexAttribute channel) where T : struct
	{
		SpriteChannelInfo channelInfo = GetChannelInfo(channel);
		byte* dataPointer = (byte*)channelInfo.buffer + channelInfo.offset;
		return NativeSliceUnsafeUtility.ConvertExistingDataToNativeSlice<T>(dataPointer, channelInfo.stride, channelInfo.count);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void SetSegmentCount(int geomCount);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void SetMeshDataCount(int vertexCount, int indexCount);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void SetMeshChannelInfo(int vertexCount, int indexCount, int hotChannelMask);

	private SpriteChannelInfo GetDataInfo(SpriteShapeDataType arrayType)
	{
		GetDataInfo_Injected(arrayType, out var ret);
		return ret;
	}

	private SpriteChannelInfo GetChannelInfo(VertexAttribute channel)
	{
		GetChannelInfo_Injected(channel, out var ret);
		return ret;
	}

	public NativeArray<Bounds> GetBounds()
	{
		return GetNativeDataArray<Bounds>(SpriteShapeDataType.BoundingBox);
	}

	public NativeArray<SpriteShapeSegment> GetSegments(int dataSize)
	{
		SetSegmentCount(dataSize);
		return GetNativeDataArray<SpriteShapeSegment>(SpriteShapeDataType.Segment);
	}

	public void GetChannels(int dataSize, out NativeArray<ushort> indices, out NativeSlice<Vector3> vertices, out NativeSlice<Vector2> texcoords)
	{
		SetMeshDataCount(dataSize, dataSize);
		indices = GetNativeDataArray<ushort>(SpriteShapeDataType.Index);
		vertices = GetChannelDataArray<Vector3>(SpriteShapeDataType.ChannelVertex, VertexAttribute.Position);
		texcoords = GetChannelDataArray<Vector2>(SpriteShapeDataType.ChannelTexCoord0, VertexAttribute.TexCoord0);
	}

	public void GetChannels(int dataSize, out NativeArray<ushort> indices, out NativeSlice<Vector3> vertices, out NativeSlice<Vector2> texcoords, out NativeSlice<Vector4> tangents)
	{
		SetMeshChannelInfo(dataSize, dataSize, 4);
		indices = GetNativeDataArray<ushort>(SpriteShapeDataType.Index);
		vertices = GetChannelDataArray<Vector3>(SpriteShapeDataType.ChannelVertex, VertexAttribute.Position);
		texcoords = GetChannelDataArray<Vector2>(SpriteShapeDataType.ChannelTexCoord0, VertexAttribute.TexCoord0);
		tangents = GetChannelDataArray<Vector4>(SpriteShapeDataType.ChannelTangent, VertexAttribute.Tangent);
	}

	public void GetChannels(int dataSize, out NativeArray<ushort> indices, out NativeSlice<Vector3> vertices, out NativeSlice<Vector2> texcoords, out NativeSlice<Vector4> tangents, out NativeSlice<Vector3> normals)
	{
		SetMeshChannelInfo(dataSize, dataSize, 6);
		indices = GetNativeDataArray<ushort>(SpriteShapeDataType.Index);
		vertices = GetChannelDataArray<Vector3>(SpriteShapeDataType.ChannelVertex, VertexAttribute.Position);
		texcoords = GetChannelDataArray<Vector2>(SpriteShapeDataType.ChannelTexCoord0, VertexAttribute.TexCoord0);
		tangents = GetChannelDataArray<Vector4>(SpriteShapeDataType.ChannelTangent, VertexAttribute.Tangent);
		normals = GetChannelDataArray<Vector3>(SpriteShapeDataType.ChannelNormal, VertexAttribute.Normal);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_color_Injected(out Color ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_color_Injected(ref Color value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void Prepare_Injected(ref JobHandle handle, ref SpriteShapeParameters shapeParams, Sprite[] sprites);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void GetDataInfo_Injected(SpriteShapeDataType arrayType, out SpriteChannelInfo ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void GetChannelInfo_Injected(VertexAttribute channel, out SpriteChannelInfo ret);
}
