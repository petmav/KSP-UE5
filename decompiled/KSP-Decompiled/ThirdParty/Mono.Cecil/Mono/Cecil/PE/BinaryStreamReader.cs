using System.IO;

namespace Mono.Cecil.PE
{
	internal class BinaryStreamReader : BinaryReader
	{
		public BinaryStreamReader(Stream stream)
			: base(stream)
		{
		}

		protected void Advance(int bytes)
		{
			BaseStream.Seek(bytes, SeekOrigin.Current);
		}

		protected DataDirectory ReadDataDirectory()
		{
			return new DataDirectory(ReadUInt32(), ReadUInt32());
		}
	}
}
