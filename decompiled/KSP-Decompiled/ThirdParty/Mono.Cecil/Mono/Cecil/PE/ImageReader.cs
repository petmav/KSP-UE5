using System;
using System.IO;
using Mono.Cecil.Metadata;

namespace Mono.Cecil.PE
{
	internal sealed class ImageReader : BinaryStreamReader
	{
		private readonly Image image;

		private DataDirectory cli;

		private DataDirectory metadata;

		public ImageReader(Stream stream)
			: base(stream)
		{
			image = new Image();
			image.FileName = stream.GetFullyQualifiedName();
		}

		private void MoveTo(DataDirectory directory)
		{
			BaseStream.Position = image.ResolveVirtualAddress(directory.VirtualAddress);
		}

		private void MoveTo(uint position)
		{
			BaseStream.Position = position;
		}

		private void ReadImage()
		{
			if (BaseStream.Length < 128)
			{
				throw new BadImageFormatException();
			}
			if (ReadUInt16() != 23117)
			{
				throw new BadImageFormatException();
			}
			Advance(58);
			MoveTo(ReadUInt32());
			if (ReadUInt32() != 17744)
			{
				throw new BadImageFormatException();
			}
			image.Architecture = ReadArchitecture();
			ushort count = ReadUInt16();
			Advance(14);
			ushort characteristics = ReadUInt16();
			ReadOptionalHeaders(out var subsystem, out var dll_characteristics);
			ReadSections(count);
			ReadCLIHeader();
			ReadMetadata();
			image.Kind = GetModuleKind(characteristics, subsystem);
			image.Characteristics = (ModuleCharacteristics)dll_characteristics;
		}

		private TargetArchitecture ReadArchitecture()
		{
			switch (ReadUInt16())
			{
			case 332:
				return TargetArchitecture.I386;
			case 34404:
				return TargetArchitecture.AMD64;
			case 512:
				return TargetArchitecture.IA64;
			case 452:
				return TargetArchitecture.ARMv7;
			default:
				throw new NotSupportedException();
			}
		}

		private static ModuleKind GetModuleKind(ushort characteristics, ushort subsystem)
		{
			if ((characteristics & 0x2000u) != 0)
			{
				return ModuleKind.Dll;
			}
			if (subsystem == 2 || subsystem == 9)
			{
				return ModuleKind.Windows;
			}
			return ModuleKind.Console;
		}

		private void ReadOptionalHeaders(out ushort subsystem, out ushort dll_characteristics)
		{
			bool flag = ReadUInt16() == 523;
			Advance(66);
			subsystem = ReadUInt16();
			dll_characteristics = ReadUInt16();
			Advance(flag ? 88 : 72);
			image.Debug = ReadDataDirectory();
			Advance(56);
			cli = ReadDataDirectory();
			if (cli.IsZero)
			{
				throw new BadImageFormatException();
			}
			Advance(8);
		}

		private string ReadAlignedString(int length)
		{
			int num = 0;
			char[] array = new char[length];
			while (num < length)
			{
				byte b = ReadByte();
				if (b == 0)
				{
					break;
				}
				array[num++] = (char)b;
			}
			Advance(-1 + ((num + 4) & -4) - num);
			return new string(array, 0, num);
		}

		private string ReadZeroTerminatedString(int length)
		{
			int num = 0;
			char[] array = new char[length];
			byte[] array2 = ReadBytes(length);
			while (num < length)
			{
				byte b = array2[num];
				if (b == 0)
				{
					break;
				}
				array[num++] = (char)b;
			}
			return new string(array, 0, num);
		}

		private void ReadSections(ushort count)
		{
			Section[] array = new Section[count];
			for (int i = 0; i < count; i++)
			{
				Section section = new Section();
				section.Name = ReadZeroTerminatedString(8);
				Advance(4);
				section.VirtualAddress = ReadUInt32();
				section.SizeOfRawData = ReadUInt32();
				section.PointerToRawData = ReadUInt32();
				Advance(16);
				array[i] = section;
				ReadSectionData(section);
			}
			image.Sections = array;
		}

		private void ReadSectionData(Section section)
		{
			long position = BaseStream.Position;
			MoveTo(section.PointerToRawData);
			int sizeOfRawData = (int)section.SizeOfRawData;
			byte[] array = new byte[sizeOfRawData];
			int num = 0;
			int num2;
			while ((num2 = Read(array, num, sizeOfRawData - num)) > 0)
			{
				num += num2;
			}
			section.Data = array;
			BaseStream.Position = position;
		}

		private void ReadCLIHeader()
		{
			MoveTo(cli);
			Advance(8);
			metadata = ReadDataDirectory();
			image.Attributes = (ModuleAttributes)ReadUInt32();
			image.EntryPointToken = ReadUInt32();
			image.Resources = ReadDataDirectory();
			image.StrongName = ReadDataDirectory();
		}

		private void ReadMetadata()
		{
			MoveTo(metadata);
			if (ReadUInt32() != 1112167234)
			{
				throw new BadImageFormatException();
			}
			Advance(8);
			image.RuntimeVersion = ReadZeroTerminatedString(ReadInt32());
			Advance(2);
			ushort num = ReadUInt16();
			Section sectionAtVirtualAddress = image.GetSectionAtVirtualAddress(metadata.VirtualAddress);
			if (sectionAtVirtualAddress == null)
			{
				throw new BadImageFormatException();
			}
			image.MetadataSection = sectionAtVirtualAddress;
			for (int i = 0; i < num; i++)
			{
				ReadMetadataStream(sectionAtVirtualAddress);
			}
			if (image.TableHeap != null)
			{
				ReadTableHeap();
			}
		}

		private void ReadMetadataStream(Section section)
		{
			uint start = metadata.VirtualAddress - section.VirtualAddress + ReadUInt32();
			uint size = ReadUInt32();
			switch (ReadAlignedString(16))
			{
			case "#~":
			case "#-":
				image.TableHeap = new TableHeap(section, start, size);
				break;
			case "#Strings":
				image.StringHeap = new StringHeap(section, start, size);
				break;
			case "#Blob":
				image.BlobHeap = new BlobHeap(section, start, size);
				break;
			case "#GUID":
				image.GuidHeap = new GuidHeap(section, start, size);
				break;
			case "#US":
				image.UserStringHeap = new UserStringHeap(section, start, size);
				break;
			}
		}

		private void ReadTableHeap()
		{
			TableHeap tableHeap = image.TableHeap;
			uint pointerToRawData = tableHeap.Section.PointerToRawData;
			MoveTo(tableHeap.Offset + pointerToRawData);
			Advance(6);
			byte sizes = ReadByte();
			Advance(1);
			tableHeap.Valid = ReadInt64();
			tableHeap.Sorted = ReadInt64();
			for (int i = 0; i < 45; i++)
			{
				if (tableHeap.HasTable((Table)i))
				{
					tableHeap.Tables[i].Length = ReadUInt32();
				}
			}
			SetIndexSize(image.StringHeap, sizes, 1);
			SetIndexSize(image.GuidHeap, sizes, 2);
			SetIndexSize(image.BlobHeap, sizes, 4);
			ComputeTableInformations();
		}

		private static void SetIndexSize(Heap heap, uint sizes, byte flag)
		{
			if (heap != null)
			{
				heap.IndexSize = (((sizes & flag) != 0) ? 4 : 2);
			}
		}

		private int GetTableIndexSize(Table table)
		{
			return image.GetTableIndexSize(table);
		}

		private int GetCodedIndexSize(CodedIndex index)
		{
			return image.GetCodedIndexSize(index);
		}

		private void ComputeTableInformations()
		{
			uint num = (uint)(int)BaseStream.Position - image.MetadataSection.PointerToRawData;
			int indexSize = image.StringHeap.IndexSize;
			int num2 = ((image.BlobHeap != null) ? image.BlobHeap.IndexSize : 2);
			TableHeap tableHeap = image.TableHeap;
			TableInformation[] tables = tableHeap.Tables;
			for (int i = 0; i < 45; i++)
			{
				Table table = (Table)i;
				if (tableHeap.HasTable(table))
				{
					int num3;
					switch (table)
					{
					case Table.Module:
						num3 = 2 + indexSize + image.GuidHeap.IndexSize * 3;
						break;
					case Table.TypeRef:
						num3 = GetCodedIndexSize(CodedIndex.ResolutionScope) + indexSize * 2;
						break;
					case Table.TypeDef:
						num3 = 4 + indexSize * 2 + GetCodedIndexSize(CodedIndex.TypeDefOrRef) + GetTableIndexSize(Table.Field) + GetTableIndexSize(Table.Method);
						break;
					case Table.FieldPtr:
						num3 = GetTableIndexSize(Table.Field);
						break;
					case Table.Field:
						num3 = 2 + indexSize + num2;
						break;
					case Table.MethodPtr:
						num3 = GetTableIndexSize(Table.Method);
						break;
					case Table.Method:
						num3 = 8 + indexSize + num2 + GetTableIndexSize(Table.Param);
						break;
					case Table.ParamPtr:
						num3 = GetTableIndexSize(Table.Param);
						break;
					case Table.Param:
						num3 = 4 + indexSize;
						break;
					case Table.InterfaceImpl:
						num3 = GetTableIndexSize(Table.TypeDef) + GetCodedIndexSize(CodedIndex.TypeDefOrRef);
						break;
					case Table.MemberRef:
						num3 = GetCodedIndexSize(CodedIndex.MemberRefParent) + indexSize + num2;
						break;
					case Table.Constant:
						num3 = 2 + GetCodedIndexSize(CodedIndex.HasConstant) + num2;
						break;
					case Table.CustomAttribute:
						num3 = GetCodedIndexSize(CodedIndex.HasCustomAttribute) + GetCodedIndexSize(CodedIndex.CustomAttributeType) + num2;
						break;
					case Table.FieldMarshal:
						num3 = GetCodedIndexSize(CodedIndex.HasFieldMarshal) + num2;
						break;
					case Table.DeclSecurity:
						num3 = 2 + GetCodedIndexSize(CodedIndex.HasDeclSecurity) + num2;
						break;
					case Table.ClassLayout:
						num3 = 6 + GetTableIndexSize(Table.TypeDef);
						break;
					case Table.FieldLayout:
						num3 = 4 + GetTableIndexSize(Table.Field);
						break;
					case Table.StandAloneSig:
						num3 = num2;
						break;
					case Table.EventMap:
						num3 = GetTableIndexSize(Table.TypeDef) + GetTableIndexSize(Table.Event);
						break;
					case Table.EventPtr:
						num3 = GetTableIndexSize(Table.Event);
						break;
					case Table.Event:
						num3 = 2 + indexSize + GetCodedIndexSize(CodedIndex.TypeDefOrRef);
						break;
					case Table.PropertyMap:
						num3 = GetTableIndexSize(Table.TypeDef) + GetTableIndexSize(Table.Property);
						break;
					case Table.PropertyPtr:
						num3 = GetTableIndexSize(Table.Property);
						break;
					case Table.Property:
						num3 = 2 + indexSize + num2;
						break;
					case Table.MethodSemantics:
						num3 = 2 + GetTableIndexSize(Table.Method) + GetCodedIndexSize(CodedIndex.HasSemantics);
						break;
					case Table.MethodImpl:
						num3 = GetTableIndexSize(Table.TypeDef) + GetCodedIndexSize(CodedIndex.MethodDefOrRef) + GetCodedIndexSize(CodedIndex.MethodDefOrRef);
						break;
					case Table.ModuleRef:
						num3 = indexSize;
						break;
					case Table.TypeSpec:
						num3 = num2;
						break;
					case Table.ImplMap:
						num3 = 2 + GetCodedIndexSize(CodedIndex.MemberForwarded) + indexSize + GetTableIndexSize(Table.ModuleRef);
						break;
					case Table.FieldRVA:
						num3 = 4 + GetTableIndexSize(Table.Field);
						break;
					case Table.EncLog:
						num3 = 8;
						break;
					case Table.EncMap:
						num3 = 4;
						break;
					case Table.Assembly:
						num3 = 16 + num2 + indexSize * 2;
						break;
					case Table.AssemblyProcessor:
						num3 = 4;
						break;
					case Table.AssemblyOS:
						num3 = 12;
						break;
					case Table.AssemblyRef:
						num3 = 12 + num2 * 2 + indexSize * 2;
						break;
					case Table.AssemblyRefProcessor:
						num3 = 4 + GetTableIndexSize(Table.AssemblyRef);
						break;
					case Table.AssemblyRefOS:
						num3 = 12 + GetTableIndexSize(Table.AssemblyRef);
						break;
					case Table.File:
						num3 = 4 + indexSize + num2;
						break;
					case Table.ExportedType:
						num3 = 8 + indexSize * 2 + GetCodedIndexSize(CodedIndex.Implementation);
						break;
					case Table.ManifestResource:
						num3 = 8 + indexSize + GetCodedIndexSize(CodedIndex.Implementation);
						break;
					case Table.NestedClass:
						num3 = GetTableIndexSize(Table.TypeDef) + GetTableIndexSize(Table.TypeDef);
						break;
					case Table.GenericParam:
						num3 = 4 + GetCodedIndexSize(CodedIndex.TypeOrMethodDef) + indexSize;
						break;
					case Table.MethodSpec:
						num3 = GetCodedIndexSize(CodedIndex.MethodDefOrRef) + num2;
						break;
					case Table.GenericParamConstraint:
						num3 = GetTableIndexSize(Table.GenericParam) + GetCodedIndexSize(CodedIndex.TypeDefOrRef);
						break;
					default:
						throw new NotSupportedException();
					}
					tables[i].RowSize = (uint)num3;
					tables[i].Offset = num;
					num += (uint)(num3 * (int)tables[i].Length);
				}
			}
		}

		public static Image ReadImageFrom(Stream stream)
		{
			try
			{
				ImageReader imageReader = new ImageReader(stream);
				imageReader.ReadImage();
				return imageReader.image;
			}
			catch (EndOfStreamException inner)
			{
				throw new BadImageFormatException(stream.GetFullyQualifiedName(), inner);
			}
		}
	}
}
