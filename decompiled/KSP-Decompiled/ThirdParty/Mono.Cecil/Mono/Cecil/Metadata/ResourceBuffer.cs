using Mono.Cecil.PE;

namespace Mono.Cecil.Metadata
{
	internal sealed class ResourceBuffer : ByteBuffer
	{
		public ResourceBuffer()
			: base(0)
		{
		}

		public uint AddResource(byte[] resource)
		{
			uint result = (uint)position;
			WriteInt32(resource.Length);
			WriteBytes(resource);
			return result;
		}
	}
}
