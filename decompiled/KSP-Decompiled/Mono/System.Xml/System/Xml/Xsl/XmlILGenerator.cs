using System.Collections.Generic;
using System.Reflection;
using System.Reflection.Emit;
using System.Xml.XPath;
using System.Xml.Xsl.IlGen;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl;

internal class XmlILGenerator
{
	private QilExpression qil;

	private GenerateHelper helper;

	private XmlILOptimizerVisitor optVisitor;

	private XmlILVisitor xmlIlVisitor;

	private XmlILModule module;

	public XmlILCommand Generate(QilExpression query, TypeBuilder typeBldr)
	{
		qil = query;
		bool useLRE = !qil.IsDebug && typeBldr == null;
		bool isDebug = qil.IsDebug;
		optVisitor = new XmlILOptimizerVisitor(qil, !qil.IsDebug);
		qil = optVisitor.Optimize();
		XmlILModule.CreateModulePermissionSet.Assert();
		if (typeBldr != null)
		{
			module = new XmlILModule(typeBldr);
		}
		else
		{
			module = new XmlILModule(useLRE, isDebug);
		}
		helper = new GenerateHelper(module, qil.IsDebug);
		CreateHelperFunctions();
		MethodInfo methExec = module.DefineMethod("Execute", typeof(void), new Type[0], new string[0], XmlILMethodAttributes.NonUser);
		XmlILMethodAttributes xmlAttrs = ((qil.Root.SourceLine == null) ? XmlILMethodAttributes.NonUser : XmlILMethodAttributes.None);
		MethodInfo methRoot = module.DefineMethod("Root", typeof(void), new Type[0], new string[0], xmlAttrs);
		foreach (EarlyBoundInfo earlyBoundType in qil.EarlyBoundTypes)
		{
			helper.StaticData.DeclareEarlyBound(earlyBoundType.NamespaceUri, earlyBoundType.EarlyBoundType);
		}
		CreateFunctionMetadata(qil.FunctionList);
		CreateGlobalValueMetadata(qil.GlobalVariableList);
		CreateGlobalValueMetadata(qil.GlobalParameterList);
		GenerateExecuteFunction(methExec, methRoot);
		xmlIlVisitor = new XmlILVisitor();
		xmlIlVisitor.Visit(qil, helper, methRoot);
		XmlQueryStaticData staticData = new XmlQueryStaticData(qil.DefaultWriterSettings, qil.WhitespaceRules, helper.StaticData);
		if (typeBldr != null)
		{
			CreateTypeInitializer(staticData);
			module.BakeMethods();
			return null;
		}
		module.BakeMethods();
		return new XmlILCommand((ExecuteDelegate)module.CreateDelegate("Execute", typeof(ExecuteDelegate)), staticData);
	}

	private void CreateFunctionMetadata(IList<QilNode> funcList)
	{
		foreach (QilFunction func in funcList)
		{
			Type[] array = new Type[func.Arguments.Count];
			string[] array2 = new string[func.Arguments.Count];
			for (int i = 0; i < func.Arguments.Count; i++)
			{
				QilParameter qilParameter = (QilParameter)func.Arguments[i];
				array[i] = XmlILTypeHelper.GetStorageType(qilParameter.XmlType);
				if (qilParameter.DebugName != null)
				{
					array2[i] = qilParameter.DebugName;
				}
			}
			Type returnType = ((!XmlILConstructInfo.Read(func).PushToWriterLast) ? XmlILTypeHelper.GetStorageType(func.XmlType) : typeof(void));
			XmlILMethodAttributes xmlAttrs = ((func.SourceLine == null) ? XmlILMethodAttributes.NonUser : XmlILMethodAttributes.None);
			MethodInfo functionBinding = module.DefineMethod(func.DebugName, returnType, array, array2, xmlAttrs);
			for (int j = 0; j < func.Arguments.Count; j++)
			{
				XmlILAnnotation.Write(func.Arguments[j]).ArgumentPosition = j;
			}
			XmlILAnnotation.Write(func).FunctionBinding = functionBinding;
		}
	}

	private void CreateGlobalValueMetadata(IList<QilNode> globalList)
	{
		foreach (QilReference global in globalList)
		{
			Type storageType = XmlILTypeHelper.GetStorageType(global.XmlType);
			XmlILMethodAttributes xmlAttrs = ((global.SourceLine == null) ? XmlILMethodAttributes.NonUser : XmlILMethodAttributes.None);
			MethodInfo functionBinding = module.DefineMethod(global.DebugName.ToString(), storageType, new Type[0], new string[0], xmlAttrs);
			XmlILAnnotation.Write(global).FunctionBinding = functionBinding;
		}
	}

	private MethodInfo GenerateExecuteFunction(MethodInfo methExec, MethodInfo methRoot)
	{
		helper.MethodBegin(methExec, null, initWriters: false);
		EvaluateGlobalValues(qil.GlobalVariableList);
		EvaluateGlobalValues(qil.GlobalParameterList);
		helper.LoadQueryRuntime();
		helper.Call(methRoot);
		helper.MethodEnd();
		return methExec;
	}

	private void CreateHelperFunctions()
	{
		MethodInfo methInfo = module.DefineMethod("SyncToNavigator", typeof(XPathNavigator), new Type[2]
		{
			typeof(XPathNavigator),
			typeof(XPathNavigator)
		}, new string[2], (XmlILMethodAttributes)3);
		helper.MethodBegin(methInfo, null, initWriters: false);
		Label label = helper.DefineLabel();
		helper.Emit(OpCodes.Ldarg_0);
		helper.Emit(OpCodes.Brfalse, label);
		helper.Emit(OpCodes.Ldarg_0);
		helper.Emit(OpCodes.Ldarg_1);
		helper.Call(XmlILMethods.NavMoveTo);
		helper.Emit(OpCodes.Brfalse, label);
		helper.Emit(OpCodes.Ldarg_0);
		helper.Emit(OpCodes.Ret);
		helper.MarkLabel(label);
		helper.Emit(OpCodes.Ldarg_1);
		helper.Call(XmlILMethods.NavClone);
		helper.MethodEnd();
	}

	private void EvaluateGlobalValues(IList<QilNode> iterList)
	{
		foreach (QilIterator iter in iterList)
		{
			if (qil.IsDebug || OptimizerPatterns.Read(iter).MatchesPattern(OptimizerPatternName.MaybeSideEffects))
			{
				MethodInfo functionBinding = XmlILAnnotation.Write(iter).FunctionBinding;
				helper.LoadQueryRuntime();
				helper.Call(functionBinding);
				helper.Emit(OpCodes.Pop);
			}
		}
	}

	public void CreateTypeInitializer(XmlQueryStaticData staticData)
	{
		staticData.GetObjectData(out var data, out var ebTypes);
		FieldInfo fldInfo = module.DefineInitializedData("__staticData", data);
		FieldInfo fldInfo2 = module.DefineField("staticData", typeof(object));
		FieldInfo fldInfo3 = module.DefineField("ebTypes", typeof(Type[]));
		ConstructorInfo methInfo = module.DefineTypeInitializer();
		helper.MethodBegin(methInfo, null, initWriters: false);
		helper.LoadInteger(data.Length);
		helper.Emit(OpCodes.Newarr, typeof(byte));
		helper.Emit(OpCodes.Dup);
		helper.Emit(OpCodes.Ldtoken, fldInfo);
		helper.Call(XmlILMethods.InitializeArray);
		helper.Emit(OpCodes.Stsfld, fldInfo2);
		if (ebTypes != null)
		{
			LocalBuilder locBldr = helper.DeclareLocal("$$$types", typeof(Type[]));
			helper.LoadInteger(ebTypes.Length);
			helper.Emit(OpCodes.Newarr, typeof(Type));
			helper.Emit(OpCodes.Stloc, locBldr);
			for (int i = 0; i < ebTypes.Length; i++)
			{
				helper.Emit(OpCodes.Ldloc, locBldr);
				helper.LoadInteger(i);
				helper.LoadType(ebTypes[i]);
				helper.Emit(OpCodes.Stelem_Ref);
			}
			helper.Emit(OpCodes.Ldloc, locBldr);
			helper.Emit(OpCodes.Stsfld, fldInfo3);
		}
		helper.MethodEnd();
	}
}
