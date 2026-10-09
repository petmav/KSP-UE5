using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections;
using System.Collections.Generic;
using System.Configuration;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Xml.Serialization.Configuration;

namespace System.Xml.Serialization;

internal class CodeGenerator
{
	internal class WhileState
	{
		public Label StartLabel;

		public Label CondLabel;

		public Label EndLabel;

		public WhileState(CodeGenerator ilg)
		{
			StartLabel = ilg.DefineLabel();
			CondLabel = ilg.DefineLabel();
			EndLabel = ilg.DefineLabel();
		}
	}

	internal static BindingFlags InstancePublicBindingFlags = BindingFlags.Instance | BindingFlags.Public;

	internal static BindingFlags InstanceBindingFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;

	internal static BindingFlags StaticBindingFlags = BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;

	internal static MethodAttributes PublicMethodAttributes = MethodAttributes.Public | MethodAttributes.HideBySig;

	internal static MethodAttributes PublicOverrideMethodAttributes = MethodAttributes.Public | MethodAttributes.Virtual | MethodAttributes.HideBySig;

	internal static MethodAttributes ProtectedOverrideMethodAttributes = MethodAttributes.Family | MethodAttributes.Virtual | MethodAttributes.HideBySig;

	internal static MethodAttributes PrivateMethodAttributes = MethodAttributes.Private | MethodAttributes.HideBySig;

	internal static Type[] EmptyTypeArray = new Type[0];

	internal static string[] EmptyStringArray = new string[0];

	private TypeBuilder typeBuilder;

	private MethodBuilder methodBuilder;

	private ILGenerator ilGen;

	private Dictionary<string, ArgBuilder> argList;

	private LocalScope currentScope;

	private Dictionary<Tuple<Type, string>, Queue<LocalBuilder>> freeLocals;

	private Stack blockStack;

	private Label methodEndLabel;

	internal LocalBuilder retLocal;

	internal Label retLabel;

	private Dictionary<Type, LocalBuilder> TmpLocals = new Dictionary<Type, LocalBuilder>();

	private static OpCode[] BranchCodes = new OpCode[6]
	{
		OpCodes.Bge,
		OpCodes.Bne_Un,
		OpCodes.Bgt,
		OpCodes.Ble,
		OpCodes.Beq,
		OpCodes.Blt
	};

	private Stack leaveLabels = new Stack();

	private static OpCode[] LdindOpCodes = new OpCode[19]
	{
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Ldind_I1,
		OpCodes.Ldind_I2,
		OpCodes.Ldind_I1,
		OpCodes.Ldind_U1,
		OpCodes.Ldind_I2,
		OpCodes.Ldind_U2,
		OpCodes.Ldind_I4,
		OpCodes.Ldind_U4,
		OpCodes.Ldind_I8,
		OpCodes.Ldind_I8,
		OpCodes.Ldind_R4,
		OpCodes.Ldind_R8,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Ldind_Ref
	};

	private static OpCode[] LdelemOpCodes = new OpCode[19]
	{
		OpCodes.Nop,
		OpCodes.Ldelem_Ref,
		OpCodes.Ldelem_Ref,
		OpCodes.Ldelem_I1,
		OpCodes.Ldelem_I2,
		OpCodes.Ldelem_I1,
		OpCodes.Ldelem_U1,
		OpCodes.Ldelem_I2,
		OpCodes.Ldelem_U2,
		OpCodes.Ldelem_I4,
		OpCodes.Ldelem_U4,
		OpCodes.Ldelem_I8,
		OpCodes.Ldelem_I8,
		OpCodes.Ldelem_R4,
		OpCodes.Ldelem_R8,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Ldelem_Ref
	};

	private static OpCode[] StelemOpCodes = new OpCode[19]
	{
		OpCodes.Nop,
		OpCodes.Stelem_Ref,
		OpCodes.Stelem_Ref,
		OpCodes.Stelem_I1,
		OpCodes.Stelem_I2,
		OpCodes.Stelem_I1,
		OpCodes.Stelem_I1,
		OpCodes.Stelem_I2,
		OpCodes.Stelem_I2,
		OpCodes.Stelem_I4,
		OpCodes.Stelem_I4,
		OpCodes.Stelem_I8,
		OpCodes.Stelem_I8,
		OpCodes.Stelem_R4,
		OpCodes.Stelem_R8,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Stelem_Ref
	};

	private static OpCode[] ConvOpCodes = new OpCode[19]
	{
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Conv_I1,
		OpCodes.Conv_I2,
		OpCodes.Conv_I1,
		OpCodes.Conv_U1,
		OpCodes.Conv_I2,
		OpCodes.Conv_U2,
		OpCodes.Conv_I4,
		OpCodes.Conv_U4,
		OpCodes.Conv_I8,
		OpCodes.Conv_U8,
		OpCodes.Conv_R4,
		OpCodes.Conv_R8,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop,
		OpCodes.Nop
	};

	private static string tempFilesLocation = null;

	private int initElseIfStack = -1;

	private IfState elseIfState;

	private int initIfStack = -1;

	private Stack whileStack;

	internal MethodBuilder MethodBuilder => methodBuilder;

	internal LocalBuilder ReturnLocal
	{
		get
		{
			if (retLocal == null)
			{
				retLocal = DeclareLocal(methodBuilder.ReturnType, "_ret");
			}
			return retLocal;
		}
	}

	internal Label ReturnLabel => retLabel;

	internal static string TempFilesLocation
	{
		get
		{
			if (tempFilesLocation == null)
			{
				object section = ConfigurationManager.GetSection(ConfigurationStrings.XmlSerializerSectionPath);
				string text = null;
				if (section != null && section is XmlSerializerSection xmlSerializerSection)
				{
					text = xmlSerializerSection.TempFilesLocation;
				}
				if (text != null)
				{
					tempFilesLocation = text.Trim();
				}
				else
				{
					tempFilesLocation = Path.GetTempPath();
				}
			}
			return tempFilesLocation;
		}
		set
		{
			tempFilesLocation = value;
		}
	}

	internal static bool IsValidLanguageIndependentIdentifier(string ident)
	{
		return System.CodeDom.Compiler.CodeGenerator.IsValidLanguageIndependentIdentifier(ident);
	}

	internal static void ValidateIdentifiers(CodeObject e)
	{
		System.CodeDom.Compiler.CodeGenerator.ValidateIdentifiers(e);
	}

	internal CodeGenerator(TypeBuilder typeBuilder)
	{
		this.typeBuilder = typeBuilder;
	}

	internal static bool IsNullableGenericType(Type type)
	{
		return type.Name == "Nullable`1";
	}

	internal static void AssertHasInterface(Type type, Type iType)
	{
	}

	internal void BeginMethod(Type returnType, string methodName, Type[] argTypes, string[] argNames, MethodAttributes methodAttributes)
	{
		methodBuilder = typeBuilder.DefineMethod(methodName, methodAttributes, returnType, argTypes);
		ilGen = methodBuilder.GetILGenerator();
		InitILGeneration(argTypes, argNames, (methodBuilder.Attributes & MethodAttributes.Static) == MethodAttributes.Static);
	}

	internal void BeginMethod(Type returnType, MethodBuilderInfo methodBuilderInfo, Type[] argTypes, string[] argNames, MethodAttributes methodAttributes)
	{
		methodBuilder = methodBuilderInfo.MethodBuilder;
		ilGen = methodBuilder.GetILGenerator();
		InitILGeneration(argTypes, argNames, (methodBuilder.Attributes & MethodAttributes.Static) == MethodAttributes.Static);
	}

	private void InitILGeneration(Type[] argTypes, string[] argNames, bool isStatic)
	{
		methodEndLabel = ilGen.DefineLabel();
		retLabel = ilGen.DefineLabel();
		blockStack = new Stack();
		whileStack = new Stack();
		currentScope = new LocalScope();
		freeLocals = new Dictionary<Tuple<Type, string>, Queue<LocalBuilder>>();
		argList = new Dictionary<string, ArgBuilder>();
		if (!isStatic)
		{
			argList.Add("this", new ArgBuilder("this", 0, typeBuilder.BaseType));
		}
		for (int i = 0; i < argTypes.Length; i++)
		{
			ArgBuilder argBuilder = new ArgBuilder(argNames[i], argList.Count, argTypes[i]);
			argList.Add(argBuilder.Name, argBuilder);
			methodBuilder.DefineParameter(argBuilder.Index, ParameterAttributes.None, argBuilder.Name);
		}
	}

	internal MethodBuilder EndMethod()
	{
		MarkLabel(methodEndLabel);
		Ret();
		MethodBuilder result = methodBuilder;
		methodBuilder = null;
		ilGen = null;
		freeLocals = null;
		blockStack = null;
		whileStack = null;
		argList = null;
		currentScope = null;
		retLocal = null;
		return result;
	}

	internal static Exception NotSupported(string msg)
	{
		return new NotSupportedException(msg);
	}

	internal ArgBuilder GetArg(string name)
	{
		return argList[name];
	}

	internal LocalBuilder GetLocal(string name)
	{
		return currentScope[name];
	}

	internal LocalBuilder GetTempLocal(Type type)
	{
		if (!TmpLocals.TryGetValue(type, out var value))
		{
			value = DeclareLocal(type, "_tmp" + TmpLocals.Count);
			TmpLocals.Add(type, value);
		}
		return value;
	}

	internal Type GetVariableType(object var)
	{
		if (var is ArgBuilder)
		{
			return ((ArgBuilder)var).ArgType;
		}
		if (var is LocalBuilder)
		{
			return ((LocalBuilder)var).LocalType;
		}
		return var.GetType();
	}

	internal object GetVariable(string name)
	{
		if (TryGetVariable(name, out var variable))
		{
			return variable;
		}
		return null;
	}

	internal bool TryGetVariable(string name, out object variable)
	{
		if (currentScope != null && currentScope.TryGetValue(name, out var value))
		{
			variable = value;
			return true;
		}
		if (argList != null && argList.TryGetValue(name, out var value2))
		{
			variable = value2;
			return true;
		}
		if (int.TryParse(name, out var result))
		{
			variable = result;
			return true;
		}
		variable = null;
		return false;
	}

	internal void EnterScope()
	{
		LocalScope localScope = new LocalScope(currentScope);
		currentScope = localScope;
	}

	internal void ExitScope()
	{
		currentScope.AddToFreeLocals(freeLocals);
		currentScope = currentScope.parent;
	}

	private bool TryDequeueLocal(Type type, string name, out LocalBuilder local)
	{
		Tuple<Type, string> key = new Tuple<Type, string>(type, name);
		if (freeLocals.TryGetValue(key, out var value))
		{
			local = value.Dequeue();
			if (value.Count == 0)
			{
				freeLocals.Remove(key);
			}
			return true;
		}
		local = null;
		return false;
	}

	internal LocalBuilder DeclareLocal(Type type, string name)
	{
		if (!TryDequeueLocal(type, name, out var local))
		{
			local = ilGen.DeclareLocal(type, pinned: false);
			if (DiagnosticsSwitches.KeepTempFiles.Enabled)
			{
				local.SetLocalSymInfo(name);
			}
		}
		currentScope[name] = local;
		return local;
	}

	internal LocalBuilder DeclareOrGetLocal(Type type, string name)
	{
		if (!currentScope.TryGetValue(name, out var value))
		{
			return DeclareLocal(type, name);
		}
		return value;
	}

	internal object For(LocalBuilder local, object start, object end)
	{
		ForState forState = new ForState(local, DefineLabel(), DefineLabel(), end);
		if (forState.Index != null)
		{
			Load(start);
			Stloc(forState.Index);
			Br(forState.TestLabel);
		}
		MarkLabel(forState.BeginLabel);
		blockStack.Push(forState);
		return forState;
	}

	internal void EndFor()
	{
		ForState forState = blockStack.Pop() as ForState;
		if (forState.Index != null)
		{
			Ldloc(forState.Index);
			Ldc(1);
			Add();
			Stloc(forState.Index);
			MarkLabel(forState.TestLabel);
			Ldloc(forState.Index);
			Load(forState.End);
			if (GetVariableType(forState.End).IsArray)
			{
				Ldlen();
			}
			else
			{
				MethodInfo method = typeof(ICollection).GetMethod("get_Count", InstanceBindingFlags, null, EmptyTypeArray, null);
				Call(method);
			}
			Blt(forState.BeginLabel);
		}
		else
		{
			Br(forState.BeginLabel);
		}
	}

	internal void If()
	{
		InternalIf(negate: false);
	}

	internal void IfNot()
	{
		InternalIf(negate: true);
	}

	private OpCode GetBranchCode(Cmp cmp)
	{
		return BranchCodes[(int)cmp];
	}

	internal void If(Cmp cmpOp)
	{
		IfState ifState = new IfState();
		ifState.EndIf = DefineLabel();
		ifState.ElseBegin = DefineLabel();
		ilGen.Emit(GetBranchCode(cmpOp), ifState.ElseBegin);
		blockStack.Push(ifState);
	}

	internal void If(object value1, Cmp cmpOp, object value2)
	{
		Load(value1);
		Load(value2);
		If(cmpOp);
	}

	internal void Else()
	{
		IfState ifState = PopIfState();
		Br(ifState.EndIf);
		MarkLabel(ifState.ElseBegin);
		ifState.ElseBegin = ifState.EndIf;
		blockStack.Push(ifState);
	}

	internal void EndIf()
	{
		IfState ifState = PopIfState();
		if (!ifState.ElseBegin.Equals(ifState.EndIf))
		{
			MarkLabel(ifState.ElseBegin);
		}
		MarkLabel(ifState.EndIf);
	}

	internal void BeginExceptionBlock()
	{
		leaveLabels.Push(DefineLabel());
		ilGen.BeginExceptionBlock();
	}

	internal void BeginCatchBlock(Type exception)
	{
		ilGen.BeginCatchBlock(exception);
	}

	internal void EndExceptionBlock()
	{
		ilGen.EndExceptionBlock();
		ilGen.MarkLabel((Label)leaveLabels.Pop());
	}

	internal void Leave()
	{
		ilGen.Emit(OpCodes.Leave, (Label)leaveLabels.Peek());
	}

	internal void Call(MethodInfo methodInfo)
	{
		if (methodInfo.IsVirtual && !methodInfo.DeclaringType.IsValueType)
		{
			ilGen.Emit(OpCodes.Callvirt, methodInfo);
		}
		else
		{
			ilGen.Emit(OpCodes.Call, methodInfo);
		}
	}

	internal void Call(ConstructorInfo ctor)
	{
		ilGen.Emit(OpCodes.Call, ctor);
	}

	internal void New(ConstructorInfo constructorInfo)
	{
		ilGen.Emit(OpCodes.Newobj, constructorInfo);
	}

	internal void InitObj(Type valueType)
	{
		ilGen.Emit(OpCodes.Initobj, valueType);
	}

	internal void NewArray(Type elementType, object len)
	{
		Load(len);
		ilGen.Emit(OpCodes.Newarr, elementType);
	}

	internal void LoadArrayElement(object obj, object arrayIndex)
	{
		Type elementType = GetVariableType(obj).GetElementType();
		Load(obj);
		Load(arrayIndex);
		if (IsStruct(elementType))
		{
			Ldelema(elementType);
			Ldobj(elementType);
		}
		else
		{
			Ldelem(elementType);
		}
	}

	internal void StoreArrayElement(object obj, object arrayIndex, object value)
	{
		Type variableType = GetVariableType(obj);
		if (variableType == typeof(Array))
		{
			Load(obj);
			Call(typeof(Array).GetMethod("SetValue", new Type[2]
			{
				typeof(object),
				typeof(int)
			}));
			return;
		}
		Type elementType = variableType.GetElementType();
		Load(obj);
		Load(arrayIndex);
		if (IsStruct(elementType))
		{
			Ldelema(elementType);
		}
		Load(value);
		ConvertValue(GetVariableType(value), elementType);
		if (IsStruct(elementType))
		{
			Stobj(elementType);
		}
		else
		{
			Stelem(elementType);
		}
	}

	private static bool IsStruct(Type objType)
	{
		if (objType.IsValueType)
		{
			return !objType.IsPrimitive;
		}
		return false;
	}

	internal Type LoadMember(object obj, MemberInfo memberInfo)
	{
		if (GetVariableType(obj).IsValueType)
		{
			LoadAddress(obj);
		}
		else
		{
			Load(obj);
		}
		return LoadMember(memberInfo);
	}

	private static MethodInfo GetPropertyMethodFromBaseType(PropertyInfo propertyInfo, bool isGetter)
	{
		Type baseType = propertyInfo.DeclaringType.BaseType;
		string name = propertyInfo.Name;
		MethodInfo methodInfo = null;
		while (baseType != null)
		{
			PropertyInfo property = baseType.GetProperty(name);
			if (property != null)
			{
				methodInfo = ((!isGetter) ? property.GetSetMethod(nonPublic: true) : property.GetGetMethod(nonPublic: true));
				if (methodInfo != null)
				{
					break;
				}
			}
			baseType = baseType.BaseType;
		}
		return methodInfo;
	}

	internal Type LoadMember(MemberInfo memberInfo)
	{
		Type type = null;
		if (memberInfo.MemberType == MemberTypes.Field)
		{
			FieldInfo fieldInfo = (FieldInfo)memberInfo;
			type = fieldInfo.FieldType;
			if (fieldInfo.IsStatic)
			{
				ilGen.Emit(OpCodes.Ldsfld, fieldInfo);
			}
			else
			{
				ilGen.Emit(OpCodes.Ldfld, fieldInfo);
			}
		}
		else
		{
			PropertyInfo propertyInfo = (PropertyInfo)memberInfo;
			type = propertyInfo.PropertyType;
			if (propertyInfo != null)
			{
				MethodInfo methodInfo = propertyInfo.GetGetMethod(nonPublic: true);
				if (methodInfo == null)
				{
					methodInfo = GetPropertyMethodFromBaseType(propertyInfo, isGetter: true);
				}
				Call(methodInfo);
			}
		}
		return type;
	}

	internal Type LoadMemberAddress(MemberInfo memberInfo)
	{
		Type type = null;
		if (memberInfo.MemberType == MemberTypes.Field)
		{
			FieldInfo fieldInfo = (FieldInfo)memberInfo;
			type = fieldInfo.FieldType;
			if (fieldInfo.IsStatic)
			{
				ilGen.Emit(OpCodes.Ldsflda, fieldInfo);
			}
			else
			{
				ilGen.Emit(OpCodes.Ldflda, fieldInfo);
			}
		}
		else
		{
			PropertyInfo propertyInfo = (PropertyInfo)memberInfo;
			type = propertyInfo.PropertyType;
			if (propertyInfo != null)
			{
				MethodInfo methodInfo = propertyInfo.GetGetMethod(nonPublic: true);
				if (methodInfo == null)
				{
					methodInfo = GetPropertyMethodFromBaseType(propertyInfo, isGetter: true);
				}
				Call(methodInfo);
				LocalBuilder tempLocal = GetTempLocal(type);
				Stloc(tempLocal);
				Ldloca(tempLocal);
			}
		}
		return type;
	}

	internal void StoreMember(MemberInfo memberInfo)
	{
		if (memberInfo.MemberType == MemberTypes.Field)
		{
			FieldInfo fieldInfo = (FieldInfo)memberInfo;
			if (fieldInfo.IsStatic)
			{
				ilGen.Emit(OpCodes.Stsfld, fieldInfo);
			}
			else
			{
				ilGen.Emit(OpCodes.Stfld, fieldInfo);
			}
			return;
		}
		PropertyInfo propertyInfo = (PropertyInfo)memberInfo;
		if (propertyInfo != null)
		{
			MethodInfo methodInfo = propertyInfo.GetSetMethod(nonPublic: true);
			if (methodInfo == null)
			{
				methodInfo = GetPropertyMethodFromBaseType(propertyInfo, isGetter: false);
			}
			Call(methodInfo);
		}
	}

	internal void Load(object obj)
	{
		if (obj == null)
		{
			ilGen.Emit(OpCodes.Ldnull);
		}
		else if (obj is ArgBuilder)
		{
			Ldarg((ArgBuilder)obj);
		}
		else if (obj is LocalBuilder)
		{
			Ldloc((LocalBuilder)obj);
		}
		else
		{
			Ldc(obj);
		}
	}

	internal void LoadAddress(object obj)
	{
		if (obj is ArgBuilder)
		{
			LdargAddress((ArgBuilder)obj);
		}
		else if (obj is LocalBuilder)
		{
			LdlocAddress((LocalBuilder)obj);
		}
		else
		{
			Load(obj);
		}
	}

	internal void ConvertAddress(Type source, Type target)
	{
		InternalConvert(source, target, isAddress: true);
	}

	internal void ConvertValue(Type source, Type target)
	{
		InternalConvert(source, target, isAddress: false);
	}

	internal void Castclass(Type target)
	{
		ilGen.Emit(OpCodes.Castclass, target);
	}

	internal void Box(Type type)
	{
		ilGen.Emit(OpCodes.Box, type);
	}

	internal void Unbox(Type type)
	{
		ilGen.Emit(OpCodes.Unbox, type);
	}

	private OpCode GetLdindOpCode(TypeCode typeCode)
	{
		return LdindOpCodes[(int)typeCode];
	}

	internal void Ldobj(Type type)
	{
		OpCode ldindOpCode = GetLdindOpCode(Type.GetTypeCode(type));
		if (!ldindOpCode.Equals(OpCodes.Nop))
		{
			ilGen.Emit(ldindOpCode);
		}
		else
		{
			ilGen.Emit(OpCodes.Ldobj, type);
		}
	}

	internal void Stobj(Type type)
	{
		ilGen.Emit(OpCodes.Stobj, type);
	}

	internal void Ceq()
	{
		ilGen.Emit(OpCodes.Ceq);
	}

	internal void Clt()
	{
		ilGen.Emit(OpCodes.Clt);
	}

	internal void Cne()
	{
		Ceq();
		Ldc(0);
		Ceq();
	}

	internal void Ble(Label label)
	{
		ilGen.Emit(OpCodes.Ble, label);
	}

	internal void Throw()
	{
		ilGen.Emit(OpCodes.Throw);
	}

	internal void Ldtoken(Type t)
	{
		ilGen.Emit(OpCodes.Ldtoken, t);
	}

	internal void Ldc(object o)
	{
		Type type = o.GetType();
		if (o is Type)
		{
			Ldtoken((Type)o);
			Call(typeof(Type).GetMethod("GetTypeFromHandle", BindingFlags.Static | BindingFlags.Public, null, new Type[1] { typeof(RuntimeTypeHandle) }, null));
			return;
		}
		if (type.IsEnum)
		{
			Ldc(((IConvertible)o).ToType(Enum.GetUnderlyingType(type), null));
			return;
		}
		switch (Type.GetTypeCode(type))
		{
		case TypeCode.Boolean:
			Ldc((bool)o);
			break;
		case TypeCode.Char:
			throw new NotSupportedException("Char is not a valid schema primitive and should be treated as int in DataContract");
		case TypeCode.SByte:
		case TypeCode.Byte:
		case TypeCode.Int16:
		case TypeCode.UInt16:
			Ldc(((IConvertible)o).ToInt32(CultureInfo.InvariantCulture));
			break;
		case TypeCode.Int32:
			Ldc((int)o);
			break;
		case TypeCode.UInt32:
			Ldc((int)(uint)o);
			break;
		case TypeCode.UInt64:
			Ldc((long)(ulong)o);
			break;
		case TypeCode.Int64:
			Ldc((long)o);
			break;
		case TypeCode.Single:
			Ldc((float)o);
			break;
		case TypeCode.Double:
			Ldc((double)o);
			break;
		case TypeCode.String:
			Ldstr((string)o);
			break;
		case TypeCode.Decimal:
		{
			ConstructorInfo constructor3 = typeof(decimal).GetConstructor(InstanceBindingFlags, null, new Type[5]
			{
				typeof(int),
				typeof(int),
				typeof(int),
				typeof(bool),
				typeof(byte)
			}, null);
			int[] bits = decimal.GetBits((decimal)o);
			Ldc(bits[0]);
			Ldc(bits[1]);
			Ldc(bits[2]);
			Ldc((bits[3] & 0x80000000u) == 2147483648u);
			Ldc((byte)((bits[3] >> 16) & 0xFF));
			New(constructor3);
			break;
		}
		case TypeCode.DateTime:
		{
			ConstructorInfo constructor2 = typeof(DateTime).GetConstructor(InstanceBindingFlags, null, new Type[1] { typeof(long) }, null);
			Ldc(((DateTime)o).Ticks);
			New(constructor2);
			break;
		}
		default:
			if (type == typeof(TimeSpan) && System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				ConstructorInfo constructor = typeof(TimeSpan).GetConstructor(InstanceBindingFlags, null, new Type[1] { typeof(long) }, null);
				Ldc(((TimeSpan)o).Ticks);
				New(constructor);
				break;
			}
			throw new NotSupportedException("UnknownConstantType");
		}
	}

	internal void Ldc(bool boolVar)
	{
		if (boolVar)
		{
			ilGen.Emit(OpCodes.Ldc_I4_1);
		}
		else
		{
			ilGen.Emit(OpCodes.Ldc_I4_0);
		}
	}

	internal void Ldc(int intVar)
	{
		switch (intVar)
		{
		case -1:
			ilGen.Emit(OpCodes.Ldc_I4_M1);
			break;
		case 0:
			ilGen.Emit(OpCodes.Ldc_I4_0);
			break;
		case 1:
			ilGen.Emit(OpCodes.Ldc_I4_1);
			break;
		case 2:
			ilGen.Emit(OpCodes.Ldc_I4_2);
			break;
		case 3:
			ilGen.Emit(OpCodes.Ldc_I4_3);
			break;
		case 4:
			ilGen.Emit(OpCodes.Ldc_I4_4);
			break;
		case 5:
			ilGen.Emit(OpCodes.Ldc_I4_5);
			break;
		case 6:
			ilGen.Emit(OpCodes.Ldc_I4_6);
			break;
		case 7:
			ilGen.Emit(OpCodes.Ldc_I4_7);
			break;
		case 8:
			ilGen.Emit(OpCodes.Ldc_I4_8);
			break;
		default:
			ilGen.Emit(OpCodes.Ldc_I4, intVar);
			break;
		}
	}

	internal void Ldc(long l)
	{
		ilGen.Emit(OpCodes.Ldc_I8, l);
	}

	internal void Ldc(float f)
	{
		ilGen.Emit(OpCodes.Ldc_R4, f);
	}

	internal void Ldc(double d)
	{
		ilGen.Emit(OpCodes.Ldc_R8, d);
	}

	internal void Ldstr(string strVar)
	{
		if (strVar == null)
		{
			ilGen.Emit(OpCodes.Ldnull);
		}
		else
		{
			ilGen.Emit(OpCodes.Ldstr, strVar);
		}
	}

	internal void LdlocAddress(LocalBuilder localBuilder)
	{
		if (localBuilder.LocalType.IsValueType)
		{
			Ldloca(localBuilder);
		}
		else
		{
			Ldloc(localBuilder);
		}
	}

	internal void Ldloc(LocalBuilder localBuilder)
	{
		ilGen.Emit(OpCodes.Ldloc, localBuilder);
	}

	internal void Ldloc(string name)
	{
		LocalBuilder localBuilder = currentScope[name];
		Ldloc(localBuilder);
	}

	internal void Stloc(Type type, string name)
	{
		LocalBuilder value = null;
		if (!currentScope.TryGetValue(name, out value))
		{
			value = DeclareLocal(type, name);
		}
		Stloc(value);
	}

	internal void Stloc(LocalBuilder local)
	{
		ilGen.Emit(OpCodes.Stloc, local);
	}

	internal void Ldloc(Type type, string name)
	{
		LocalBuilder localBuilder = currentScope[name];
		Ldloc(localBuilder);
	}

	internal void Ldloca(LocalBuilder localBuilder)
	{
		ilGen.Emit(OpCodes.Ldloca, localBuilder);
	}

	internal void LdargAddress(ArgBuilder argBuilder)
	{
		if (argBuilder.ArgType.IsValueType)
		{
			Ldarga(argBuilder);
		}
		else
		{
			Ldarg(argBuilder);
		}
	}

	internal void Ldarg(string arg)
	{
		Ldarg(GetArg(arg));
	}

	internal void Ldarg(ArgBuilder arg)
	{
		Ldarg(arg.Index);
	}

	internal void Ldarg(int slot)
	{
		switch (slot)
		{
		case 0:
			ilGen.Emit(OpCodes.Ldarg_0);
			return;
		case 1:
			ilGen.Emit(OpCodes.Ldarg_1);
			return;
		case 2:
			ilGen.Emit(OpCodes.Ldarg_2);
			return;
		case 3:
			ilGen.Emit(OpCodes.Ldarg_3);
			return;
		}
		if (slot <= 255)
		{
			ilGen.Emit(OpCodes.Ldarg_S, slot);
		}
		else
		{
			ilGen.Emit(OpCodes.Ldarg, slot);
		}
	}

	internal void Ldarga(ArgBuilder argBuilder)
	{
		Ldarga(argBuilder.Index);
	}

	internal void Ldarga(int slot)
	{
		if (slot <= 255)
		{
			ilGen.Emit(OpCodes.Ldarga_S, slot);
		}
		else
		{
			ilGen.Emit(OpCodes.Ldarga, slot);
		}
	}

	internal void Ldlen()
	{
		ilGen.Emit(OpCodes.Ldlen);
		ilGen.Emit(OpCodes.Conv_I4);
	}

	private OpCode GetLdelemOpCode(TypeCode typeCode)
	{
		return LdelemOpCodes[(int)typeCode];
	}

	internal void Ldelem(Type arrayElementType)
	{
		if (arrayElementType.IsEnum)
		{
			Ldelem(Enum.GetUnderlyingType(arrayElementType));
			return;
		}
		OpCode ldelemOpCode = GetLdelemOpCode(Type.GetTypeCode(arrayElementType));
		if (ldelemOpCode.Equals(OpCodes.Nop))
		{
			throw new InvalidOperationException("ArrayTypeIsNotSupported");
		}
		ilGen.Emit(ldelemOpCode);
	}

	internal void Ldelema(Type arrayElementType)
	{
		OpCode ldelema = OpCodes.Ldelema;
		ilGen.Emit(ldelema, arrayElementType);
	}

	private OpCode GetStelemOpCode(TypeCode typeCode)
	{
		return StelemOpCodes[(int)typeCode];
	}

	internal void Stelem(Type arrayElementType)
	{
		if (arrayElementType.IsEnum)
		{
			Stelem(Enum.GetUnderlyingType(arrayElementType));
			return;
		}
		OpCode stelemOpCode = GetStelemOpCode(Type.GetTypeCode(arrayElementType));
		if (stelemOpCode.Equals(OpCodes.Nop))
		{
			throw new InvalidOperationException("ArrayTypeIsNotSupported");
		}
		ilGen.Emit(stelemOpCode);
	}

	internal Label DefineLabel()
	{
		return ilGen.DefineLabel();
	}

	internal void MarkLabel(Label label)
	{
		ilGen.MarkLabel(label);
	}

	internal void Nop()
	{
		ilGen.Emit(OpCodes.Nop);
	}

	internal void Add()
	{
		ilGen.Emit(OpCodes.Add);
	}

	internal void Ret()
	{
		ilGen.Emit(OpCodes.Ret);
	}

	internal void Br(Label label)
	{
		ilGen.Emit(OpCodes.Br, label);
	}

	internal void Br_S(Label label)
	{
		ilGen.Emit(OpCodes.Br_S, label);
	}

	internal void Blt(Label label)
	{
		ilGen.Emit(OpCodes.Blt, label);
	}

	internal void Brfalse(Label label)
	{
		ilGen.Emit(OpCodes.Brfalse, label);
	}

	internal void Brtrue(Label label)
	{
		ilGen.Emit(OpCodes.Brtrue, label);
	}

	internal void Pop()
	{
		ilGen.Emit(OpCodes.Pop);
	}

	internal void Dup()
	{
		ilGen.Emit(OpCodes.Dup);
	}

	internal void Ldftn(MethodInfo methodInfo)
	{
		ilGen.Emit(OpCodes.Ldftn, methodInfo);
	}

	private void InternalIf(bool negate)
	{
		IfState ifState = new IfState();
		ifState.EndIf = DefineLabel();
		ifState.ElseBegin = DefineLabel();
		if (negate)
		{
			Brtrue(ifState.ElseBegin);
		}
		else
		{
			Brfalse(ifState.ElseBegin);
		}
		blockStack.Push(ifState);
	}

	private OpCode GetConvOpCode(TypeCode typeCode)
	{
		return ConvOpCodes[(int)typeCode];
	}

	private void InternalConvert(Type source, Type target, bool isAddress)
	{
		if (target == source)
		{
			return;
		}
		if (target.IsValueType)
		{
			if (source.IsValueType)
			{
				OpCode convOpCode = GetConvOpCode(Type.GetTypeCode(target));
				if (convOpCode.Equals(OpCodes.Nop))
				{
					throw new CodeGeneratorConversionException(source, target, isAddress, "NoConversionPossibleTo");
				}
				ilGen.Emit(convOpCode);
				return;
			}
			if (!source.IsAssignableFrom(target))
			{
				throw new CodeGeneratorConversionException(source, target, isAddress, "IsNotAssignableFrom");
			}
			Unbox(target);
			if (!isAddress)
			{
				Ldobj(target);
			}
		}
		else if (target.IsAssignableFrom(source))
		{
			if (source.IsValueType)
			{
				if (isAddress)
				{
					Ldobj(source);
				}
				Box(source);
			}
		}
		else if (source.IsAssignableFrom(target))
		{
			Castclass(target);
		}
		else
		{
			if (!target.IsInterface && !source.IsInterface)
			{
				throw new CodeGeneratorConversionException(source, target, isAddress, "IsNotAssignableFrom");
			}
			Castclass(target);
		}
	}

	private IfState PopIfState()
	{
		return blockStack.Pop() as IfState;
	}

	internal static AssemblyBuilder CreateAssemblyBuilder(AppDomain appDomain, string name)
	{
		AssemblyName assemblyName = new AssemblyName();
		assemblyName.Name = name;
		assemblyName.Version = new Version(1, 0, 0, 0);
		if (DiagnosticsSwitches.KeepTempFiles.Enabled)
		{
			return appDomain.DefineDynamicAssembly(assemblyName, AssemblyBuilderAccess.RunAndSave, TempFilesLocation);
		}
		return appDomain.DefineDynamicAssembly(assemblyName, AssemblyBuilderAccess.Run);
	}

	internal static ModuleBuilder CreateModuleBuilder(AssemblyBuilder assemblyBuilder, string name)
	{
		if (DiagnosticsSwitches.KeepTempFiles.Enabled)
		{
			return assemblyBuilder.DefineDynamicModule(name, name + ".dll", emitSymbolInfo: true);
		}
		return assemblyBuilder.DefineDynamicModule(name);
	}

	internal static TypeBuilder CreateTypeBuilder(ModuleBuilder moduleBuilder, string name, TypeAttributes attributes, Type parent, Type[] interfaces)
	{
		return moduleBuilder.DefineType("Microsoft.Xml.Serialization.GeneratedAssembly." + name, attributes, parent, interfaces);
	}

	internal void InitElseIf()
	{
		elseIfState = (IfState)blockStack.Pop();
		initElseIfStack = blockStack.Count;
		Br(elseIfState.EndIf);
		MarkLabel(elseIfState.ElseBegin);
	}

	internal void InitIf()
	{
		initIfStack = blockStack.Count;
	}

	internal void AndIf(Cmp cmpOp)
	{
		if (initIfStack == blockStack.Count)
		{
			initIfStack = -1;
			If(cmpOp);
		}
		else if (initElseIfStack == blockStack.Count)
		{
			initElseIfStack = -1;
			elseIfState.ElseBegin = DefineLabel();
			ilGen.Emit(GetBranchCode(cmpOp), elseIfState.ElseBegin);
			blockStack.Push(elseIfState);
		}
		else
		{
			IfState ifState = (IfState)blockStack.Peek();
			ilGen.Emit(GetBranchCode(cmpOp), ifState.ElseBegin);
		}
	}

	internal void AndIf()
	{
		if (initIfStack == blockStack.Count)
		{
			initIfStack = -1;
			If();
		}
		else if (initElseIfStack == blockStack.Count)
		{
			initElseIfStack = -1;
			elseIfState.ElseBegin = DefineLabel();
			Brfalse(elseIfState.ElseBegin);
			blockStack.Push(elseIfState);
		}
		else
		{
			IfState ifState = (IfState)blockStack.Peek();
			Brfalse(ifState.ElseBegin);
		}
	}

	internal void IsInst(Type type)
	{
		ilGen.Emit(OpCodes.Isinst, type);
	}

	internal void Beq(Label label)
	{
		ilGen.Emit(OpCodes.Beq, label);
	}

	internal void Bne(Label label)
	{
		ilGen.Emit(OpCodes.Bne_Un, label);
	}

	internal void GotoMethodEnd()
	{
		Br(methodEndLabel);
	}

	internal void WhileBegin()
	{
		WhileState whileState = new WhileState(this);
		Br(whileState.CondLabel);
		MarkLabel(whileState.StartLabel);
		whileStack.Push(whileState);
	}

	internal void WhileEnd()
	{
		WhileState whileState = (WhileState)whileStack.Pop();
		MarkLabel(whileState.EndLabel);
	}

	internal void WhileBreak()
	{
		WhileState whileState = (WhileState)whileStack.Peek();
		Br(whileState.EndLabel);
	}

	internal void WhileContinue()
	{
		WhileState whileState = (WhileState)whileStack.Peek();
		Br(whileState.CondLabel);
	}

	internal void WhileBeginCondition()
	{
		WhileState whileState = (WhileState)whileStack.Peek();
		Nop();
		MarkLabel(whileState.CondLabel);
	}

	internal void WhileEndCondition()
	{
		WhileState whileState = (WhileState)whileStack.Peek();
		Brtrue(whileState.StartLabel);
	}
}
