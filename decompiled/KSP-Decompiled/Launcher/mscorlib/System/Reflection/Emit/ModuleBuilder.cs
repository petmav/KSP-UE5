using System.Collections.Generic;
using System.Diagnostics.SymbolStore;
using System.IO;
using System.Resources;
using System.Runtime.InteropServices;
using System.Security;
using Unity;

namespace System.Reflection.Emit;

public abstract class ModuleBuilder : Module, _ModuleBuilder
{
	public void CreateGlobalFunctions()
	{
		throw new PlatformNotSupportedException();
	}

	public EnumBuilder DefineEnum(string name, TypeAttributes visibility, Type underlyingType)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineGlobalMethod(string name, MethodAttributes attributes, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineGlobalMethod(string name, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineGlobalMethod(string name, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] requiredReturnTypeCustomModifiers, Type[] optionalReturnTypeCustomModifiers, Type[] parameterTypes, Type[][] requiredParameterTypeCustomModifiers, Type[][] optionalParameterTypeCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineInitializedData(string name, byte[] data, FieldAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name, TypeAttributes attr)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name, TypeAttributes attr, Type parent)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name, TypeAttributes attr, Type parent, int typesize)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name, TypeAttributes attr, Type parent, PackingSize packsize)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name, TypeAttributes attr, Type parent, Type[] interfaces)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineType(string name, TypeAttributes attr, Type parent, PackingSize packingSize, int typesize)
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineUninitializedData(string name, int size, FieldAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodInfo GetArrayMethod(Type arrayClass, string methodName, CallingConventions callingConvention, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
	{
		throw new PlatformNotSupportedException();
	}

	public ISymbolDocumentWriter DefineDocument(string url, Guid language, Guid languageVendor, Guid documentType)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void DefineManifestResource(string name, Stream stream, ResourceAttributes attribute)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	public MethodBuilder DefinePInvokeMethod(string name, string dllName, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes, CallingConvention nativeCallConv, CharSet nativeCharSet)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public MethodBuilder DefinePInvokeMethod(string name, string dllName, string entryName, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes, CallingConvention nativeCallConv, CharSet nativeCharSet)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IResourceWriter DefineResource(string name, string description)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IResourceWriter DefineResource(string name, string description, ResourceAttributes attribute)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void DefineUnmanagedResource(byte[] resource)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public void DefineUnmanagedResource(string resourceFileName)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public MethodToken GetArrayMethodToken(Type arrayClass, string methodName, CallingConventions callingConvention, Type returnType, Type[] parameterTypes)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodToken);
	}

	[SecuritySafeCritical]
	[ComVisible(true)]
	public MethodToken GetConstructorToken(ConstructorInfo con)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodToken);
	}

	[SecuritySafeCritical]
	public MethodToken GetConstructorToken(ConstructorInfo constructor, IEnumerable<Type> optionalParameterTypes)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodToken);
	}

	[SecuritySafeCritical]
	public FieldToken GetFieldToken(FieldInfo field)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(FieldToken);
	}

	[SecuritySafeCritical]
	public MethodToken GetMethodToken(MethodInfo method)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodToken);
	}

	[SecuritySafeCritical]
	public MethodToken GetMethodToken(MethodInfo method, IEnumerable<Type> optionalParameterTypes)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(MethodToken);
	}

	[SecuritySafeCritical]
	public SignatureToken GetSignatureToken(byte[] sigBytes, int sigLength)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(SignatureToken);
	}

	[SecuritySafeCritical]
	public SignatureToken GetSignatureToken(SignatureHelper sigHelper)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(SignatureToken);
	}

	[SecuritySafeCritical]
	public StringToken GetStringConstant(string str)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(StringToken);
	}

	public ISymbolWriter GetSymWriter()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public TypeToken GetTypeToken(string name)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(TypeToken);
	}

	[SecuritySafeCritical]
	public TypeToken GetTypeToken(Type type)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(TypeToken);
	}

	public bool IsTransient()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void SetSymCustomAttribute(string name, byte[] data)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public void SetUserEntryPoint(MethodInfo entryPoint)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ModuleBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ModuleBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ModuleBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _ModuleBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
