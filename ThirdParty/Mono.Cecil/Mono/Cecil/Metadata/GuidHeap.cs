using System;
using Mono.Cecil.PE;

namespace Mono.Cecil.Metadata
{
	internal sealed class GuidHeap : Heap
	{
		public GuidHeap(Section section, uint start, uint size)
			: base(section, start, size)
		{
		}

		public Guid Read(uint index)
		{
			if (index == 0)
			{
				return default(Guid);
			}
			byte[] array = new byte[16];
			index--;
			Buffer.BlockCopy(Section.Data, (int)(Offset + index), array, 0, 16);
			return new Guid(array);
		}
	}
}
