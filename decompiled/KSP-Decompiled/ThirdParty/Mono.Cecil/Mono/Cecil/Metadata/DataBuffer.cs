using Mono.Cecil.PE;

namespace Mono.Cecil.Metadata
{
	internal sealed class DataBuffer : ByteBuffer
	{
		public DataBuffer()
			: base(0)
		{
		}

		public uint AddData(byte[] data)
		{
			uint result = (uint)position;
			WriteBytes(data);
			return result;
		}
	}
}
