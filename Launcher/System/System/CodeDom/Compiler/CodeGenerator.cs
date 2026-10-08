using System.IO;
using System.Reflection;
using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public abstract class CodeGenerator : ICodeGenerator
{
	protected CodeTypeDeclaration CurrentClass
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected CodeTypeMember CurrentMember
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected string CurrentMemberName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected string CurrentTypeName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected int Indent
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected bool IsCurrentClass
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected bool IsCurrentDelegate
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected bool IsCurrentEnum
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected bool IsCurrentInterface
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected bool IsCurrentStruct
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected abstract string NullToken { get; }

	protected CodeGeneratorOptions Options
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected TextWriter Output
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected CodeGenerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void ContinueOnNewLine(string st)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract string CreateEscapedIdentifier(string value);

	protected abstract string CreateValidIdentifier(string value);

	protected abstract void GenerateArgumentReferenceExpression(CodeArgumentReferenceExpression e);

	protected abstract void GenerateArrayCreateExpression(CodeArrayCreateExpression e);

	protected abstract void GenerateArrayIndexerExpression(CodeArrayIndexerExpression e);

	protected abstract void GenerateAssignStatement(CodeAssignStatement e);

	protected abstract void GenerateAttachEventStatement(CodeAttachEventStatement e);

	protected abstract void GenerateAttributeDeclarationsEnd(CodeAttributeDeclarationCollection attributes);

	protected abstract void GenerateAttributeDeclarationsStart(CodeAttributeDeclarationCollection attributes);

	protected abstract void GenerateBaseReferenceExpression(CodeBaseReferenceExpression e);

	protected virtual void GenerateBinaryOperatorExpression(CodeBinaryOperatorExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateCastExpression(CodeCastExpression e);

	public virtual void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateComment(CodeComment e);

	protected virtual void GenerateCommentStatement(CodeCommentStatement e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateCommentStatements(CodeCommentStatementCollection e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateCompileUnit(CodeCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateCompileUnitEnd(CodeCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateCompileUnitStart(CodeCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateConditionStatement(CodeConditionStatement e);

	protected abstract void GenerateConstructor(CodeConstructor e, CodeTypeDeclaration c);

	protected virtual void GenerateDecimalValue(decimal d)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateDefaultValueExpression(CodeDefaultValueExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateDelegateCreateExpression(CodeDelegateCreateExpression e);

	protected abstract void GenerateDelegateInvokeExpression(CodeDelegateInvokeExpression e);

	protected virtual void GenerateDirectionExpression(CodeDirectionExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateDirectives(CodeDirectiveCollection directives)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateDoubleValue(double d)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateEntryPointMethod(CodeEntryPointMethod e, CodeTypeDeclaration c);

	protected abstract void GenerateEvent(CodeMemberEvent e, CodeTypeDeclaration c);

	protected abstract void GenerateEventReferenceExpression(CodeEventReferenceExpression e);

	protected void GenerateExpression(CodeExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateExpressionStatement(CodeExpressionStatement e);

	protected abstract void GenerateField(CodeMemberField e);

	protected abstract void GenerateFieldReferenceExpression(CodeFieldReferenceExpression e);

	protected abstract void GenerateGotoStatement(CodeGotoStatement e);

	protected abstract void GenerateIndexerExpression(CodeIndexerExpression e);

	protected abstract void GenerateIterationStatement(CodeIterationStatement e);

	protected abstract void GenerateLabeledStatement(CodeLabeledStatement e);

	protected abstract void GenerateLinePragmaEnd(CodeLinePragma e);

	protected abstract void GenerateLinePragmaStart(CodeLinePragma e);

	protected abstract void GenerateMethod(CodeMemberMethod e, CodeTypeDeclaration c);

	protected abstract void GenerateMethodInvokeExpression(CodeMethodInvokeExpression e);

	protected abstract void GenerateMethodReferenceExpression(CodeMethodReferenceExpression e);

	protected abstract void GenerateMethodReturnStatement(CodeMethodReturnStatement e);

	protected virtual void GenerateNamespace(CodeNamespace e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateNamespaceEnd(CodeNamespace e);

	protected abstract void GenerateNamespaceImport(CodeNamespaceImport e);

	protected void GenerateNamespaceImports(CodeNamespace e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void GenerateNamespaces(CodeCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateNamespaceStart(CodeNamespace e);

	protected abstract void GenerateObjectCreateExpression(CodeObjectCreateExpression e);

	protected virtual void GenerateParameterDeclarationExpression(CodeParameterDeclarationExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GeneratePrimitiveExpression(CodePrimitiveExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateProperty(CodeMemberProperty e, CodeTypeDeclaration c);

	protected abstract void GeneratePropertyReferenceExpression(CodePropertyReferenceExpression e);

	protected abstract void GeneratePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e);

	protected abstract void GenerateRemoveEventStatement(CodeRemoveEventStatement e);

	protected virtual void GenerateSingleFloatValue(float s)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateSnippetCompileUnit(CodeSnippetCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateSnippetExpression(CodeSnippetExpression e);

	protected abstract void GenerateSnippetMember(CodeSnippetTypeMember e);

	protected virtual void GenerateSnippetStatement(CodeSnippetStatement e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void GenerateStatement(CodeStatement e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void GenerateStatements(CodeStatementCollection stms)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateThisReferenceExpression(CodeThisReferenceExpression e);

	protected abstract void GenerateThrowExceptionStatement(CodeThrowExceptionStatement e);

	protected abstract void GenerateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e);

	protected abstract void GenerateTypeConstructor(CodeTypeConstructor e);

	protected abstract void GenerateTypeEnd(CodeTypeDeclaration e);

	protected virtual void GenerateTypeOfExpression(CodeTypeOfExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void GenerateTypeReferenceExpression(CodeTypeReferenceExpression e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void GenerateTypes(CodeNamespace e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void GenerateTypeStart(CodeTypeDeclaration e);

	protected abstract void GenerateVariableDeclarationStatement(CodeVariableDeclarationStatement e);

	protected abstract void GenerateVariableReferenceExpression(CodeVariableReferenceExpression e);

	protected abstract string GetTypeOutput(CodeTypeReference value);

	protected abstract bool IsValidIdentifier(string value);

	public static bool IsValidLanguageIndependentIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected virtual void OutputAttributeArgument(CodeAttributeArgument arg)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputAttributeDeclarations(CodeAttributeDeclarationCollection attributes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputDirection(FieldDirection dir)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputExpressionList(CodeExpressionCollection expressions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputExpressionList(CodeExpressionCollection expressions, bool newlineBetweenItems)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputFieldScopeModifier(MemberAttributes attributes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputIdentifier(string ident)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputMemberAccessModifier(MemberAttributes attributes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputMemberScopeModifier(MemberAttributes attributes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputOperator(CodeBinaryOperatorType op)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputParameters(CodeParameterDeclarationExpressionCollection parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract void OutputType(CodeTypeReference typeRef);

	protected virtual void OutputTypeAttributes(TypeAttributes attributes, bool isStruct, bool isEnum)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OutputTypeNamePair(CodeTypeReference typeRef, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract string QuoteSnippetString(string value);

	protected abstract bool Supports(GeneratorSupport support);

	string ICodeGenerator.CreateEscapedIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	string ICodeGenerator.CreateValidIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void ICodeGenerator.GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ICodeGenerator.GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ICodeGenerator.GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ICodeGenerator.GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ICodeGenerator.GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	string ICodeGenerator.GetTypeOutput(CodeTypeReference type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	bool ICodeGenerator.IsValidIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	bool ICodeGenerator.Supports(GeneratorSupport support)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	void ICodeGenerator.ValidateIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void ValidateIdentifier(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void ValidateIdentifiers(CodeObject e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
