using System;
using System.IO;
using System.Reflection;
using System.Runtime.Serialization;
using System.Security.Cryptography;
using System.Text;
using Mono.Cecil.Cil;
using Mono.Cecil.Metadata;
using Mono.Collections.Generic;
using Mono.Security.Cryptography;

namespace Mono.Cecil
{
	internal static class Mixin
	{
		public const int NotResolvedMarker = -2;

		public const int NoDataMarker = -1;

		internal static object NoValue = new object();

		internal static object NotResolved = new object();

		public static uint ReadCompressedUInt32(this byte[] data, ref int position)
		{
			uint result;
			if ((data[position] & 0x80) == 0)
			{
				result = data[position];
				position++;
			}
			else if ((data[position] & 0x40) == 0)
			{
				result = (uint)((data[position] & -129) << 8);
				result |= data[position + 1];
				position += 2;
			}
			else
			{
				result = (uint)((data[position] & -193) << 24);
				result |= (uint)(data[position + 1] << 16);
				result |= (uint)(data[position + 2] << 8);
				result |= data[position + 3];
				position += 4;
			}
			return result;
		}

		public static MetadataToken GetMetadataToken(this CodedIndex self, uint data)
		{
			uint rid;
			TokenType type;
			switch (self)
			{
			case CodedIndex.TypeDefOrRef:
				rid = data >> 2;
				switch (data & 3)
				{
				case 0u:
					break;
				case 1u:
					goto IL_006b;
				case 2u:
					goto IL_0076;
				default:
					goto end_IL_0003;
				}
				type = TokenType.TypeDef;
				goto IL_0443;
			case CodedIndex.HasConstant:
				rid = data >> 2;
				switch (data & 3)
				{
				case 0u:
					break;
				case 1u:
					goto IL_00ad;
				case 2u:
					goto IL_00b8;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Field;
				goto IL_0443;
			case CodedIndex.HasCustomAttribute:
				rid = data >> 5;
				switch (data & 0x1F)
				{
				case 0u:
					break;
				case 1u:
					goto IL_0134;
				case 2u:
					goto IL_013f;
				case 3u:
					goto IL_014a;
				case 4u:
					goto IL_0155;
				case 5u:
					goto IL_0160;
				case 6u:
					goto IL_016b;
				case 7u:
					goto IL_0176;
				case 8u:
					goto IL_017d;
				case 9u:
					goto IL_0188;
				case 10u:
					goto IL_0193;
				case 11u:
					goto IL_019e;
				case 12u:
					goto IL_01a9;
				case 13u:
					goto IL_01b4;
				case 14u:
					goto IL_01bf;
				case 15u:
					goto IL_01ca;
				case 16u:
					goto IL_01d5;
				case 17u:
					goto IL_01e0;
				case 18u:
					goto IL_01eb;
				case 19u:
					goto IL_01f6;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Method;
				goto IL_0443;
			case CodedIndex.HasFieldMarshal:
				rid = data >> 1;
				switch (data & 1)
				{
				case 0u:
					break;
				case 1u:
					goto IL_0229;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Field;
				goto IL_0443;
			case CodedIndex.HasDeclSecurity:
				rid = data >> 2;
				switch (data & 3)
				{
				case 0u:
					break;
				case 1u:
					goto IL_0260;
				case 2u:
					goto IL_026b;
				default:
					goto end_IL_0003;
				}
				type = TokenType.TypeDef;
				goto IL_0443;
			case CodedIndex.MemberRefParent:
				rid = data >> 3;
				switch (data & 7)
				{
				case 0u:
					break;
				case 1u:
					goto IL_02aa;
				case 2u:
					goto IL_02b5;
				case 3u:
					goto IL_02c0;
				case 4u:
					goto IL_02cb;
				default:
					goto end_IL_0003;
				}
				type = TokenType.TypeDef;
				goto IL_0443;
			case CodedIndex.HasSemantics:
				rid = data >> 1;
				switch (data & 1)
				{
				case 0u:
					break;
				case 1u:
					goto IL_02fe;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Event;
				goto IL_0443;
			case CodedIndex.MethodDefOrRef:
				rid = data >> 1;
				switch (data & 1)
				{
				case 0u:
					break;
				case 1u:
					goto IL_0331;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Method;
				goto IL_0443;
			case CodedIndex.MemberForwarded:
				rid = data >> 1;
				switch (data & 1)
				{
				case 0u:
					break;
				case 1u:
					goto IL_0364;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Field;
				goto IL_0443;
			case CodedIndex.Implementation:
				rid = data >> 2;
				switch (data & 3)
				{
				case 0u:
					break;
				case 1u:
					goto IL_039b;
				case 2u:
					goto IL_03a6;
				default:
					goto end_IL_0003;
				}
				type = TokenType.File;
				goto IL_0443;
			case CodedIndex.CustomAttributeType:
				rid = data >> 3;
				switch (data & 7)
				{
				case 2u:
					break;
				case 3u:
					goto IL_03d5;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Method;
				goto IL_0443;
			case CodedIndex.ResolutionScope:
				rid = data >> 2;
				switch (data & 3)
				{
				case 0u:
					break;
				case 1u:
					goto IL_0403;
				case 2u:
					goto IL_040b;
				case 3u:
					goto IL_0413;
				default:
					goto end_IL_0003;
				}
				type = TokenType.Module;
				goto IL_0443;
			case CodedIndex.TypeOrMethodDef:
				{
					rid = data >> 1;
					switch (data & 1)
					{
					case 0u:
						break;
					case 1u:
						goto IL_043d;
					default:
						goto end_IL_0003;
					}
					type = TokenType.TypeDef;
					goto IL_0443;
				}
				IL_043d:
				type = TokenType.Method;
				goto IL_0443;
				IL_014a:
				type = TokenType.TypeDef;
				goto IL_0443;
				IL_0413:
				type = TokenType.TypeRef;
				goto IL_0443;
				IL_040b:
				type = TokenType.AssemblyRef;
				goto IL_0443;
				IL_0403:
				type = TokenType.ModuleRef;
				goto IL_0443;
				IL_013f:
				type = TokenType.TypeRef;
				goto IL_0443;
				IL_03d5:
				type = TokenType.MemberRef;
				goto IL_0443;
				IL_0134:
				type = TokenType.Field;
				goto IL_0443;
				IL_03a6:
				type = TokenType.ExportedType;
				goto IL_0443;
				IL_039b:
				type = TokenType.AssemblyRef;
				goto IL_0443;
				IL_006b:
				type = TokenType.TypeRef;
				goto IL_0443;
				IL_0364:
				type = TokenType.Method;
				goto IL_0443;
				IL_0331:
				type = TokenType.MemberRef;
				goto IL_0443;
				IL_00b8:
				type = TokenType.Property;
				goto IL_0443;
				IL_02fe:
				type = TokenType.Property;
				goto IL_0443;
				IL_00ad:
				type = TokenType.Param;
				goto IL_0443;
				IL_02cb:
				type = TokenType.TypeSpec;
				goto IL_0443;
				IL_02c0:
				type = TokenType.Method;
				goto IL_0443;
				IL_02b5:
				type = TokenType.ModuleRef;
				goto IL_0443;
				IL_02aa:
				type = TokenType.TypeRef;
				goto IL_0443;
				IL_0443:
				return new MetadataToken(type, rid);
				IL_026b:
				type = TokenType.Assembly;
				goto IL_0443;
				IL_0260:
				type = TokenType.Method;
				goto IL_0443;
				IL_0229:
				type = TokenType.Param;
				goto IL_0443;
				IL_0076:
				type = TokenType.TypeSpec;
				goto IL_0443;
				IL_01f6:
				type = TokenType.GenericParam;
				goto IL_0443;
				IL_01eb:
				type = TokenType.ManifestResource;
				goto IL_0443;
				IL_01e0:
				type = TokenType.ExportedType;
				goto IL_0443;
				IL_01d5:
				type = TokenType.File;
				goto IL_0443;
				IL_01ca:
				type = TokenType.AssemblyRef;
				goto IL_0443;
				IL_01bf:
				type = TokenType.Assembly;
				goto IL_0443;
				IL_01b4:
				type = TokenType.TypeSpec;
				goto IL_0443;
				IL_01a9:
				type = TokenType.ModuleRef;
				goto IL_0443;
				IL_019e:
				type = TokenType.Signature;
				goto IL_0443;
				IL_0193:
				type = TokenType.Event;
				goto IL_0443;
				IL_0188:
				type = TokenType.Property;
				goto IL_0443;
				IL_017d:
				type = TokenType.Permission;
				goto IL_0443;
				IL_0176:
				type = TokenType.Module;
				goto IL_0443;
				IL_016b:
				type = TokenType.MemberRef;
				goto IL_0443;
				IL_0160:
				type = TokenType.InterfaceImpl;
				goto IL_0443;
				IL_0155:
				type = TokenType.Param;
				goto IL_0443;
				end_IL_0003:
				break;
			}
			return MetadataToken.Zero;
		}

		public static uint CompressMetadataToken(this CodedIndex self, MetadataToken token)
		{
			uint result = 0u;
			if (token.RID == 0)
			{
				return result;
			}
			switch (self)
			{
			case CodedIndex.TypeDefOrRef:
				result = token.RID << 2;
				switch (token.TokenType)
				{
				case TokenType.TypeDef:
					return result;
				case TokenType.TypeRef:
					return result | 1u;
				case TokenType.TypeSpec:
					return result | 2u;
				}
				break;
			case CodedIndex.HasConstant:
				result = token.RID << 2;
				switch (token.TokenType)
				{
				case TokenType.Field:
					return result;
				case TokenType.Param:
					return result | 1u;
				case TokenType.Property:
					return result | 2u;
				}
				break;
			case CodedIndex.HasCustomAttribute:
				result = token.RID << 5;
				switch (token.TokenType)
				{
				case TokenType.Method:
					return result;
				case TokenType.Field:
					return result | 1u;
				case TokenType.TypeRef:
					return result | 2u;
				case TokenType.TypeDef:
					return result | 3u;
				case TokenType.Param:
					return result | 4u;
				case TokenType.InterfaceImpl:
					return result | 5u;
				case TokenType.MemberRef:
					return result | 6u;
				case TokenType.Module:
					return result | 7u;
				case TokenType.Permission:
					return result | 8u;
				case TokenType.Property:
					return result | 9u;
				case TokenType.Event:
					return result | 0xAu;
				case TokenType.Signature:
					return result | 0xBu;
				case TokenType.ModuleRef:
					return result | 0xCu;
				case TokenType.TypeSpec:
					return result | 0xDu;
				case TokenType.Assembly:
					return result | 0xEu;
				case TokenType.AssemblyRef:
					return result | 0xFu;
				case TokenType.File:
					return result | 0x10u;
				case TokenType.ExportedType:
					return result | 0x11u;
				case TokenType.ManifestResource:
					return result | 0x12u;
				case TokenType.GenericParam:
					return result | 0x13u;
				}
				break;
			case CodedIndex.HasFieldMarshal:
				result = token.RID << 1;
				switch (token.TokenType)
				{
				case TokenType.Field:
					return result;
				case TokenType.Param:
					return result | 1u;
				}
				break;
			case CodedIndex.HasDeclSecurity:
				result = token.RID << 2;
				switch (token.TokenType)
				{
				case TokenType.TypeDef:
					return result;
				case TokenType.Method:
					return result | 1u;
				case TokenType.Assembly:
					return result | 2u;
				}
				break;
			case CodedIndex.MemberRefParent:
				result = token.RID << 3;
				switch (token.TokenType)
				{
				case TokenType.TypeDef:
					return result;
				case TokenType.TypeRef:
					return result | 1u;
				case TokenType.ModuleRef:
					return result | 2u;
				case TokenType.Method:
					return result | 3u;
				case TokenType.TypeSpec:
					return result | 4u;
				}
				break;
			case CodedIndex.HasSemantics:
				result = token.RID << 1;
				switch (token.TokenType)
				{
				case TokenType.Event:
					return result;
				case TokenType.Property:
					return result | 1u;
				}
				break;
			case CodedIndex.MethodDefOrRef:
				result = token.RID << 1;
				switch (token.TokenType)
				{
				case TokenType.Method:
					return result;
				case TokenType.MemberRef:
					return result | 1u;
				}
				break;
			case CodedIndex.MemberForwarded:
				result = token.RID << 1;
				switch (token.TokenType)
				{
				case TokenType.Field:
					return result;
				case TokenType.Method:
					return result | 1u;
				}
				break;
			case CodedIndex.Implementation:
				result = token.RID << 2;
				switch (token.TokenType)
				{
				case TokenType.File:
					return result;
				case TokenType.AssemblyRef:
					return result | 1u;
				case TokenType.ExportedType:
					return result | 2u;
				}
				break;
			case CodedIndex.CustomAttributeType:
				result = token.RID << 3;
				switch (token.TokenType)
				{
				case TokenType.Method:
					return result | 2u;
				case TokenType.MemberRef:
					return result | 3u;
				}
				break;
			case CodedIndex.ResolutionScope:
				result = token.RID << 2;
				switch (token.TokenType)
				{
				case TokenType.Module:
					return result;
				case TokenType.ModuleRef:
					return result | 1u;
				case TokenType.AssemblyRef:
					return result | 2u;
				case TokenType.TypeRef:
					return result | 3u;
				}
				break;
			case CodedIndex.TypeOrMethodDef:
				result = token.RID << 1;
				switch (token.TokenType)
				{
				case TokenType.TypeDef:
					return result;
				case TokenType.Method:
					return result | 1u;
				}
				break;
			}
			throw new ArgumentException();
		}

		public static int GetSize(this CodedIndex self, Func<Table, int> counter)
		{
			int num;
			Table[] array;
			switch (self)
			{
			case CodedIndex.TypeDefOrRef:
				num = 2;
				array = new Table[3]
				{
					Table.TypeDef,
					Table.TypeRef,
					Table.TypeSpec
				};
				break;
			case CodedIndex.HasConstant:
				num = 2;
				array = new Table[3]
				{
					Table.Field,
					Table.Param,
					Table.Property
				};
				break;
			case CodedIndex.HasCustomAttribute:
				num = 5;
				array = new Table[20]
				{
					Table.Method,
					Table.Field,
					Table.TypeRef,
					Table.TypeDef,
					Table.Param,
					Table.InterfaceImpl,
					Table.MemberRef,
					Table.Module,
					Table.DeclSecurity,
					Table.Property,
					Table.Event,
					Table.StandAloneSig,
					Table.ModuleRef,
					Table.TypeSpec,
					Table.Assembly,
					Table.AssemblyRef,
					Table.File,
					Table.ExportedType,
					Table.ManifestResource,
					Table.GenericParam
				};
				break;
			case CodedIndex.HasFieldMarshal:
				num = 1;
				array = new Table[2]
				{
					Table.Field,
					Table.Param
				};
				break;
			case CodedIndex.HasDeclSecurity:
				num = 2;
				array = new Table[3]
				{
					Table.TypeDef,
					Table.Method,
					Table.Assembly
				};
				break;
			case CodedIndex.MemberRefParent:
				num = 3;
				array = new Table[5]
				{
					Table.TypeDef,
					Table.TypeRef,
					Table.ModuleRef,
					Table.Method,
					Table.TypeSpec
				};
				break;
			case CodedIndex.HasSemantics:
				num = 1;
				array = new Table[2]
				{
					Table.Event,
					Table.Property
				};
				break;
			case CodedIndex.MethodDefOrRef:
				num = 1;
				array = new Table[2]
				{
					Table.Method,
					Table.MemberRef
				};
				break;
			case CodedIndex.MemberForwarded:
				num = 1;
				array = new Table[2]
				{
					Table.Field,
					Table.Method
				};
				break;
			case CodedIndex.Implementation:
				num = 2;
				array = new Table[3]
				{
					Table.File,
					Table.AssemblyRef,
					Table.ExportedType
				};
				break;
			case CodedIndex.CustomAttributeType:
				num = 3;
				array = new Table[2]
				{
					Table.Method,
					Table.MemberRef
				};
				break;
			case CodedIndex.ResolutionScope:
				num = 2;
				array = new Table[4]
				{
					Table.Module,
					Table.ModuleRef,
					Table.AssemblyRef,
					Table.TypeRef
				};
				break;
			case CodedIndex.TypeOrMethodDef:
				num = 1;
				array = new Table[2]
				{
					Table.TypeDef,
					Table.Method
				};
				break;
			default:
				throw new ArgumentException();
			}
			int num2 = 0;
			for (int i = 0; i < array.Length; i++)
			{
				num2 = System.Math.Max(counter(array[i]), num2);
			}
			if (num2 >= 1 << 16 - num)
			{
				return 4;
			}
			return 2;
		}

		public static bool GetHasSecurityDeclarations(this ISecurityDeclarationProvider self, ModuleDefinition module)
		{
			if (module.HasImage())
			{
				return module.Read(self, (ISecurityDeclarationProvider provider, MetadataReader reader) => reader.HasSecurityDeclarations(provider));
			}
			return false;
		}

		public static Collection<SecurityDeclaration> GetSecurityDeclarations(this ISecurityDeclarationProvider self, ref Collection<SecurityDeclaration> variable, ModuleDefinition module)
		{
			if (!module.HasImage())
			{
				return variable = new Collection<SecurityDeclaration>();
			}
			return module.Read(ref variable, self, (ISecurityDeclarationProvider provider, MetadataReader reader) => reader.ReadSecurityDeclarations(provider));
		}

		public static void CheckName(string name)
		{
			if (name == null)
			{
				throw new ArgumentNullException("name");
			}
			if (name.Length == 0)
			{
				throw new ArgumentException("Empty name");
			}
		}

		public static void ResolveConstant(this IConstantProvider self, ref object constant, ModuleDefinition module)
		{
			if (module == null)
			{
				constant = NoValue;
				return;
			}
			lock (module.SyncRoot)
			{
				if (constant != NotResolved)
				{
					return;
				}
				if (module.HasImage())
				{
					constant = module.Read(self, (IConstantProvider provider, MetadataReader reader) => reader.ReadConstant(provider));
				}
				else
				{
					constant = NoValue;
				}
			}
		}

		public static bool GetHasCustomAttributes(this ICustomAttributeProvider self, ModuleDefinition module)
		{
			if (module.HasImage())
			{
				return module.Read(self, (ICustomAttributeProvider provider, MetadataReader reader) => reader.HasCustomAttributes(provider));
			}
			return false;
		}

		public static Collection<CustomAttribute> GetCustomAttributes(this ICustomAttributeProvider self, ref Collection<CustomAttribute> variable, ModuleDefinition module)
		{
			if (!module.HasImage())
			{
				return variable = new Collection<CustomAttribute>();
			}
			return module.Read(ref variable, self, (ICustomAttributeProvider provider, MetadataReader reader) => reader.ReadCustomAttributes(provider));
		}

		public static bool ContainsGenericParameter(this IGenericInstance self)
		{
			Collection<TypeReference> genericArguments = self.GenericArguments;
			for (int i = 0; i < genericArguments.Count; i++)
			{
				if (genericArguments[i].ContainsGenericParameter)
				{
					return true;
				}
			}
			return false;
		}

		public static void GenericInstanceFullName(this IGenericInstance self, StringBuilder builder)
		{
			builder.Append("<");
			Collection<TypeReference> genericArguments = self.GenericArguments;
			for (int i = 0; i < genericArguments.Count; i++)
			{
				if (i > 0)
				{
					builder.Append(",");
				}
				builder.Append(genericArguments[i].FullName);
			}
			builder.Append(">");
		}

		public static bool GetHasGenericParameters(this IGenericParameterProvider self, ModuleDefinition module)
		{
			if (module.HasImage())
			{
				return module.Read(self, (IGenericParameterProvider provider, MetadataReader reader) => reader.HasGenericParameters(provider));
			}
			return false;
		}

		public static Collection<GenericParameter> GetGenericParameters(this IGenericParameterProvider self, ref Collection<GenericParameter> collection, ModuleDefinition module)
		{
			if (!module.HasImage())
			{
				return collection = new GenericParameterCollection(self);
			}
			return module.Read(ref collection, self, (IGenericParameterProvider provider, MetadataReader reader) => reader.ReadGenericParameters(provider));
		}

		public static bool GetHasMarshalInfo(this IMarshalInfoProvider self, ModuleDefinition module)
		{
			if (module.HasImage())
			{
				return module.Read(self, (IMarshalInfoProvider provider, MetadataReader reader) => reader.HasMarshalInfo(provider));
			}
			return false;
		}

		public static MarshalInfo GetMarshalInfo(this IMarshalInfoProvider self, ref MarshalInfo variable, ModuleDefinition module)
		{
			if (!module.HasImage())
			{
				return null;
			}
			return module.Read(ref variable, self, (IMarshalInfoProvider provider, MetadataReader reader) => reader.ReadMarshalInfo(provider));
		}

		public static void CheckModifier(TypeReference modifierType, TypeReference type)
		{
			if (modifierType == null)
			{
				throw new ArgumentNullException("modifierType");
			}
			if (type == null)
			{
				throw new ArgumentNullException("type");
			}
		}

		public static bool HasImplicitThis(this IMethodSignature self)
		{
			if (self.HasThis)
			{
				return !self.ExplicitThis;
			}
			return false;
		}

		public static void MethodSignatureFullName(this IMethodSignature self, StringBuilder builder)
		{
			builder.Append("(");
			if (self.HasParameters)
			{
				Collection<ParameterDefinition> parameters = self.Parameters;
				for (int i = 0; i < parameters.Count; i++)
				{
					ParameterDefinition parameterDefinition = parameters[i];
					if (i > 0)
					{
						builder.Append(",");
					}
					if (parameterDefinition.ParameterType.IsSentinel)
					{
						builder.Append("...,");
					}
					builder.Append(parameterDefinition.ParameterType.FullName);
				}
			}
			builder.Append(")");
		}

		public static bool GetAttributes(this uint self, uint attributes)
		{
			return (self & attributes) != 0;
		}

		public static uint SetAttributes(this uint self, uint attributes, bool value)
		{
			if (value)
			{
				return self | attributes;
			}
			return self & ~attributes;
		}

		public static bool GetMaskedAttributes(this uint self, uint mask, uint attributes)
		{
			return (self & mask) == attributes;
		}

		public static uint SetMaskedAttributes(this uint self, uint mask, uint attributes, bool value)
		{
			if (value)
			{
				self &= ~mask;
				return self | attributes;
			}
			return self & ~(mask & attributes);
		}

		public static bool GetAttributes(this ushort self, ushort attributes)
		{
			return (self & attributes) != 0;
		}

		public static ushort SetAttributes(this ushort self, ushort attributes, bool value)
		{
			if (value)
			{
				return (ushort)(self | attributes);
			}
			return (ushort)(self & ~attributes);
		}

		public static bool GetMaskedAttributes(this ushort self, ushort mask, uint attributes)
		{
			return (self & mask) == attributes;
		}

		public static ushort SetMaskedAttributes(this ushort self, ushort mask, uint attributes, bool value)
		{
			if (value)
			{
				self = (ushort)(self & ~mask);
				return (ushort)(self | attributes);
			}
			return (ushort)(self & ~(mask & attributes));
		}

		public static ParameterDefinition GetParameter(this Mono.Cecil.Cil.MethodBody self, int index)
		{
			MethodDefinition method = self.method;
			if (method.HasThis)
			{
				if (index == 0)
				{
					return self.ThisParameter;
				}
				index--;
			}
			Collection<ParameterDefinition> parameters = method.Parameters;
			if (index < 0 || index >= parameters.size)
			{
				return null;
			}
			return parameters[index];
		}

		public static VariableDefinition GetVariable(this Mono.Cecil.Cil.MethodBody self, int index)
		{
			Collection<VariableDefinition> variables = self.Variables;
			if (index < 0 || index >= variables.size)
			{
				return null;
			}
			return variables[index];
		}

		public static bool GetSemantics(this MethodDefinition self, MethodSemanticsAttributes semantics)
		{
			return (self.SemanticsAttributes & semantics) != 0;
		}

		public static void SetSemantics(this MethodDefinition self, MethodSemanticsAttributes semantics, bool value)
		{
			if (value)
			{
				self.SemanticsAttributes |= semantics;
			}
			else
			{
				self.SemanticsAttributes &= (MethodSemanticsAttributes)(ushort)(~(int)semantics);
			}
		}

		public static bool IsVarArg(this IMethodSignature self)
		{
			return (self.CallingConvention & MethodCallingConvention.VarArg) != 0;
		}

		public static int GetSentinelPosition(this IMethodSignature self)
		{
			if (!self.HasParameters)
			{
				return -1;
			}
			Collection<ParameterDefinition> parameters = self.Parameters;
			for (int i = 0; i < parameters.Count; i++)
			{
				if (parameters[i].ParameterType.IsSentinel)
				{
					return i;
				}
			}
			return -1;
		}

		public static void CheckParameters(object parameters)
		{
			if (parameters == null)
			{
				throw new ArgumentNullException("parameters");
			}
		}

		public static bool HasImage(this ModuleDefinition self)
		{
			return self?.HasImage ?? false;
		}

		public static bool IsCorlib(this ModuleDefinition module)
		{
			if (module.Assembly == null)
			{
				return false;
			}
			return module.Assembly.Name.Name == "mscorlib";
		}

		public static string GetFullyQualifiedName(this Stream self)
		{
			if (!(self is FileStream fileStream))
			{
				return string.Empty;
			}
			return Path.GetFullPath(fileStream.Name);
		}

		public static TargetRuntime ParseRuntime(this string self)
		{
			switch (self[1])
			{
			case '1':
				if (self[3] != '0')
				{
					return TargetRuntime.Net_1_1;
				}
				return TargetRuntime.Net_1_0;
			case '2':
				return TargetRuntime.Net_2_0;
			default:
				return TargetRuntime.Net_4_0;
			}
		}

		public static string RuntimeVersionString(this TargetRuntime runtime)
		{
			switch (runtime)
			{
			case TargetRuntime.Net_1_0:
				return "v1.0.3705";
			case TargetRuntime.Net_1_1:
				return "v1.1.4322";
			case TargetRuntime.Net_2_0:
				return "v2.0.50727";
			default:
				return "v4.0.30319";
			}
		}

		public static TypeReference GetEnumUnderlyingType(this TypeDefinition self)
		{
			Collection<FieldDefinition> fields = self.Fields;
			for (int i = 0; i < fields.Count; i++)
			{
				FieldDefinition fieldDefinition = fields[i];
				if (!fieldDefinition.IsStatic)
				{
					return fieldDefinition.FieldType;
				}
			}
			throw new ArgumentException();
		}

		public static TypeDefinition GetNestedType(this TypeDefinition self, string fullname)
		{
			if (!self.HasNestedTypes)
			{
				return null;
			}
			Collection<TypeDefinition> nestedTypes = self.NestedTypes;
			for (int i = 0; i < nestedTypes.Count; i++)
			{
				TypeDefinition typeDefinition = nestedTypes[i];
				if (typeDefinition.TypeFullName() == fullname)
				{
					return typeDefinition;
				}
			}
			return null;
		}

		public static bool IsPrimitive(this ElementType self)
		{
			switch (self)
			{
			case ElementType.Boolean:
			case ElementType.Char:
			case ElementType.I1:
			case ElementType.U1:
			case ElementType.I2:
			case ElementType.U2:
			case ElementType.I4:
			case ElementType.U4:
			case ElementType.I8:
			case ElementType.U8:
			case ElementType.R4:
			case ElementType.R8:
			case ElementType.I:
			case ElementType.U:
				return true;
			default:
				return false;
			}
		}

		public static string TypeFullName(this TypeReference self)
		{
			if (!string.IsNullOrEmpty(self.Namespace))
			{
				return self.Namespace + '.' + self.Name;
			}
			return self.Name;
		}

		public static bool IsTypeOf(this TypeReference self, string @namespace, string name)
		{
			if (self.Name == name)
			{
				return self.Namespace == @namespace;
			}
			return false;
		}

		public static bool IsTypeSpecification(this TypeReference type)
		{
			switch (type.etype)
			{
			case ElementType.Ptr:
			case ElementType.ByRef:
			case ElementType.Var:
			case ElementType.Array:
			case ElementType.GenericInst:
			case ElementType.FnPtr:
			case ElementType.SzArray:
			case ElementType.MVar:
			case ElementType.CModReqD:
			case ElementType.CModOpt:
			case ElementType.Sentinel:
			case ElementType.Pinned:
				return true;
			default:
				return false;
			}
		}

		public static TypeDefinition CheckedResolve(this TypeReference self)
		{
			TypeDefinition typeDefinition = self.Resolve();
			if (typeDefinition == null)
			{
				throw new ResolutionException(self);
			}
			return typeDefinition;
		}

		public static void CheckType(TypeReference type)
		{
			if (type == null)
			{
				throw new ArgumentNullException("type");
			}
		}

		public static RSA CreateRSA(this StrongNameKeyPair key_pair)
		{
			if (!TryGetKeyContainer(key_pair, out var key, out var key_container))
			{
				return Mono.Security.Cryptography.CryptoConvert.FromCapiKeyBlob(key);
			}
			CspParameters cspParameters = new CspParameters();
			cspParameters.Flags = CspProviderFlags.UseMachineKeyStore;
			cspParameters.KeyContainerName = key_container;
			cspParameters.KeyNumber = 2;
			CspParameters parameters = cspParameters;
			return new RSACryptoServiceProvider(parameters);
		}

		private static bool TryGetKeyContainer(ISerializable key_pair, out byte[] key, out string key_container)
		{
			SerializationInfo serializationInfo = new SerializationInfo(typeof(StrongNameKeyPair), new FormatterConverter());
			key_pair.GetObjectData(serializationInfo, default(StreamingContext));
			key = (byte[])serializationInfo.GetValue("_keyPairArray", typeof(byte[]));
			key_container = serializationInfo.GetString("_keyPairContainer");
			return key_container != null;
		}

		public static bool IsNullOrEmpty<T>(this T[] self)
		{
			if (self != null)
			{
				return self.Length == 0;
			}
			return true;
		}

		public static bool IsNullOrEmpty<T>(this Collection<T> self)
		{
			if (self != null)
			{
				return self.size == 0;
			}
			return true;
		}

		public static T[] Resize<T>(this T[] self, int length)
		{
			Array.Resize(ref self, length);
			return self;
		}
	}
}
