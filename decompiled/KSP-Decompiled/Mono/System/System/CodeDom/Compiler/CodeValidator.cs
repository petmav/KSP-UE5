using System.IO;

namespace System.CodeDom.Compiler;

internal sealed class CodeValidator
{
	private static readonly char[] s_newLineChars = new char[5] { '\r', '\n', '\u2028', '\u2029', '\u0085' };

	private CodeTypeDeclaration _currentClass;

	private bool IsCurrentInterface
	{
		get
		{
			if (_currentClass == null || _currentClass is CodeTypeDelegate)
			{
				return false;
			}
			return _currentClass.IsInterface;
		}
	}

	private bool IsCurrentEnum
	{
		get
		{
			if (_currentClass == null || _currentClass is CodeTypeDelegate)
			{
				return false;
			}
			return _currentClass.IsEnum;
		}
	}

	private bool IsCurrentDelegate
	{
		get
		{
			if (_currentClass != null)
			{
				return _currentClass is CodeTypeDelegate;
			}
			return false;
		}
	}

	internal void ValidateIdentifiers(CodeObject e)
	{
		if (e is CodeCompileUnit)
		{
			ValidateCodeCompileUnit((CodeCompileUnit)e);
			return;
		}
		if (e is CodeComment)
		{
			ValidateComment((CodeComment)e);
			return;
		}
		if (e is CodeExpression)
		{
			ValidateExpression((CodeExpression)e);
			return;
		}
		if (e is CodeNamespace)
		{
			ValidateNamespace((CodeNamespace)e);
			return;
		}
		if (e is CodeNamespaceImport)
		{
			ValidateNamespaceImport((CodeNamespaceImport)e);
			return;
		}
		if (e is CodeStatement)
		{
			ValidateStatement((CodeStatement)e);
			return;
		}
		if (e is CodeTypeMember)
		{
			ValidateTypeMember((CodeTypeMember)e);
			return;
		}
		if (e is CodeTypeReference)
		{
			ValidateTypeReference((CodeTypeReference)e);
			return;
		}
		if (e is CodeDirective)
		{
			ValidateCodeDirective((CodeDirective)e);
			return;
		}
		throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
	}

	private void ValidateTypeMember(CodeTypeMember e)
	{
		ValidateCommentStatements(e.Comments);
		ValidateCodeDirectives(e.StartDirectives);
		ValidateCodeDirectives(e.EndDirectives);
		if (e.LinePragma != null)
		{
			ValidateLinePragmaStart(e.LinePragma);
		}
		if (e is CodeMemberEvent)
		{
			ValidateEvent((CodeMemberEvent)e);
			return;
		}
		if (e is CodeMemberField)
		{
			ValidateField((CodeMemberField)e);
			return;
		}
		if (e is CodeMemberMethod)
		{
			ValidateMemberMethod((CodeMemberMethod)e);
			return;
		}
		if (e is CodeMemberProperty)
		{
			ValidateProperty((CodeMemberProperty)e);
			return;
		}
		if (e is CodeSnippetTypeMember)
		{
			ValidateSnippetMember((CodeSnippetTypeMember)e);
			return;
		}
		if (e is CodeTypeDeclaration)
		{
			ValidateTypeDeclaration((CodeTypeDeclaration)e);
			return;
		}
		throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
	}

	private void ValidateCodeCompileUnit(CodeCompileUnit e)
	{
		ValidateCodeDirectives(e.StartDirectives);
		ValidateCodeDirectives(e.EndDirectives);
		if (e is CodeSnippetCompileUnit)
		{
			ValidateSnippetCompileUnit((CodeSnippetCompileUnit)e);
			return;
		}
		ValidateCompileUnitStart(e);
		ValidateNamespaces(e);
		ValidateCompileUnitEnd(e);
	}

	private void ValidateSnippetCompileUnit(CodeSnippetCompileUnit e)
	{
		if (e.LinePragma != null)
		{
			ValidateLinePragmaStart(e.LinePragma);
		}
	}

	private void ValidateCompileUnitStart(CodeCompileUnit e)
	{
		if (e.AssemblyCustomAttributes.Count > 0)
		{
			ValidateAttributes(e.AssemblyCustomAttributes);
		}
	}

	private void ValidateCompileUnitEnd(CodeCompileUnit e)
	{
	}

	private void ValidateNamespaces(CodeCompileUnit e)
	{
		foreach (CodeNamespace @namespace in e.Namespaces)
		{
			ValidateNamespace(@namespace);
		}
	}

	private void ValidateNamespace(CodeNamespace e)
	{
		ValidateCommentStatements(e.Comments);
		ValidateNamespaceStart(e);
		ValidateNamespaceImports(e);
		ValidateTypes(e);
	}

	private static void ValidateNamespaceStart(CodeNamespace e)
	{
		if (!string.IsNullOrEmpty(e.Name))
		{
			ValidateTypeName(e, "Name", e.Name);
		}
	}

	private void ValidateNamespaceImports(CodeNamespace e)
	{
		foreach (CodeNamespaceImport import in e.Imports)
		{
			if (import.LinePragma != null)
			{
				ValidateLinePragmaStart(import.LinePragma);
			}
			ValidateNamespaceImport(import);
		}
	}

	private static void ValidateNamespaceImport(CodeNamespaceImport e)
	{
		ValidateTypeName(e, "Namespace", e.Namespace);
	}

	private void ValidateAttributes(CodeAttributeDeclarationCollection attributes)
	{
		if (attributes.Count == 0)
		{
			return;
		}
		foreach (CodeAttributeDeclaration attribute in attributes)
		{
			ValidateTypeName(attribute, "Name", attribute.Name);
			ValidateTypeReference(attribute.AttributeType);
			foreach (CodeAttributeArgument argument in attribute.Arguments)
			{
				ValidateAttributeArgument(argument);
			}
		}
	}

	private void ValidateAttributeArgument(CodeAttributeArgument arg)
	{
		if (!string.IsNullOrEmpty(arg.Name))
		{
			ValidateIdentifier(arg, "Name", arg.Name);
		}
		ValidateExpression(arg.Value);
	}

	private void ValidateTypes(CodeNamespace e)
	{
		foreach (CodeTypeDeclaration type in e.Types)
		{
			ValidateTypeDeclaration(type);
		}
	}

	private void ValidateTypeDeclaration(CodeTypeDeclaration e)
	{
		CodeTypeDeclaration currentClass = _currentClass;
		_currentClass = e;
		ValidateTypeStart(e);
		ValidateTypeParameters(e.TypeParameters);
		ValidateTypeMembers(e);
		ValidateTypeReferences(e.BaseTypes);
		_currentClass = currentClass;
	}

	private void ValidateTypeMembers(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			ValidateTypeMember(member);
		}
	}

	private void ValidateTypeParameters(CodeTypeParameterCollection parameters)
	{
		for (int i = 0; i < parameters.Count; i++)
		{
			ValidateTypeParameter(parameters[i]);
		}
	}

	private void ValidateTypeParameter(CodeTypeParameter e)
	{
		ValidateIdentifier(e, "Name", e.Name);
		ValidateTypeReferences(e.Constraints);
		ValidateAttributes(e.CustomAttributes);
	}

	private void ValidateField(CodeMemberField e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		ValidateIdentifier(e, "Name", e.Name);
		if (!IsCurrentEnum)
		{
			ValidateTypeReference(e.Type);
		}
		if (e.InitExpression != null)
		{
			ValidateExpression(e.InitExpression);
		}
	}

	private void ValidateConstructor(CodeConstructor e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		ValidateParameters(e.Parameters);
		CodeExpressionCollection baseConstructorArgs = e.BaseConstructorArgs;
		CodeExpressionCollection chainedConstructorArgs = e.ChainedConstructorArgs;
		if (baseConstructorArgs.Count > 0)
		{
			ValidateExpressionList(baseConstructorArgs);
		}
		if (chainedConstructorArgs.Count > 0)
		{
			ValidateExpressionList(chainedConstructorArgs);
		}
		ValidateStatements(e.Statements);
	}

	private void ValidateProperty(CodeMemberProperty e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		ValidateTypeReference(e.Type);
		ValidateTypeReferences(e.ImplementationTypes);
		if (e.PrivateImplementationType != null && !IsCurrentInterface)
		{
			ValidateTypeReference(e.PrivateImplementationType);
		}
		if (e.Parameters.Count > 0 && string.Equals(e.Name, "Item", StringComparison.OrdinalIgnoreCase))
		{
			ValidateParameters(e.Parameters);
		}
		else
		{
			ValidateIdentifier(e, "Name", e.Name);
		}
		if (e.HasGet && !IsCurrentInterface && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract)
		{
			ValidateStatements(e.GetStatements);
		}
		if (e.HasSet && !IsCurrentInterface && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract)
		{
			ValidateStatements(e.SetStatements);
		}
	}

	private void ValidateMemberMethod(CodeMemberMethod e)
	{
		ValidateCommentStatements(e.Comments);
		if (e.LinePragma != null)
		{
			ValidateLinePragmaStart(e.LinePragma);
		}
		ValidateTypeParameters(e.TypeParameters);
		ValidateTypeReferences(e.ImplementationTypes);
		if (e is CodeEntryPointMethod)
		{
			ValidateStatements(((CodeEntryPointMethod)e).Statements);
		}
		else if (e is CodeConstructor)
		{
			ValidateConstructor((CodeConstructor)e);
		}
		else if (e is CodeTypeConstructor)
		{
			ValidateTypeConstructor((CodeTypeConstructor)e);
		}
		else
		{
			ValidateMethod(e);
		}
	}

	private void ValidateTypeConstructor(CodeTypeConstructor e)
	{
		ValidateStatements(e.Statements);
	}

	private void ValidateMethod(CodeMemberMethod e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		if (e.ReturnTypeCustomAttributes.Count > 0)
		{
			ValidateAttributes(e.ReturnTypeCustomAttributes);
		}
		ValidateTypeReference(e.ReturnType);
		if (e.PrivateImplementationType != null)
		{
			ValidateTypeReference(e.PrivateImplementationType);
		}
		ValidateIdentifier(e, "Name", e.Name);
		ValidateParameters(e.Parameters);
		if (!IsCurrentInterface && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract)
		{
			ValidateStatements(e.Statements);
		}
	}

	private void ValidateSnippetMember(CodeSnippetTypeMember e)
	{
	}

	private void ValidateTypeStart(CodeTypeDeclaration e)
	{
		ValidateCommentStatements(e.Comments);
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		ValidateIdentifier(e, "Name", e.Name);
		if (IsCurrentDelegate)
		{
			CodeTypeDelegate codeTypeDelegate = (CodeTypeDelegate)e;
			ValidateTypeReference(codeTypeDelegate.ReturnType);
			ValidateParameters(codeTypeDelegate.Parameters);
			return;
		}
		foreach (CodeTypeReference baseType in e.BaseTypes)
		{
			ValidateTypeReference(baseType);
		}
	}

	private void ValidateCommentStatements(CodeCommentStatementCollection e)
	{
		foreach (CodeCommentStatement item in e)
		{
			ValidateCommentStatement(item);
		}
	}

	private void ValidateCommentStatement(CodeCommentStatement e)
	{
		ValidateComment(e.Comment);
	}

	private void ValidateComment(CodeComment e)
	{
	}

	private void ValidateStatement(CodeStatement e)
	{
		ValidateCodeDirectives(e.StartDirectives);
		ValidateCodeDirectives(e.EndDirectives);
		if (e is CodeCommentStatement)
		{
			ValidateCommentStatement((CodeCommentStatement)e);
			return;
		}
		if (e is CodeMethodReturnStatement)
		{
			ValidateMethodReturnStatement((CodeMethodReturnStatement)e);
			return;
		}
		if (e is CodeConditionStatement)
		{
			ValidateConditionStatement((CodeConditionStatement)e);
			return;
		}
		if (e is CodeTryCatchFinallyStatement)
		{
			ValidateTryCatchFinallyStatement((CodeTryCatchFinallyStatement)e);
			return;
		}
		if (e is CodeAssignStatement)
		{
			ValidateAssignStatement((CodeAssignStatement)e);
			return;
		}
		if (e is CodeExpressionStatement)
		{
			ValidateExpressionStatement((CodeExpressionStatement)e);
			return;
		}
		if (e is CodeIterationStatement)
		{
			ValidateIterationStatement((CodeIterationStatement)e);
			return;
		}
		if (e is CodeThrowExceptionStatement)
		{
			ValidateThrowExceptionStatement((CodeThrowExceptionStatement)e);
			return;
		}
		if (e is CodeSnippetStatement)
		{
			ValidateSnippetStatement((CodeSnippetStatement)e);
			return;
		}
		if (e is CodeVariableDeclarationStatement)
		{
			ValidateVariableDeclarationStatement((CodeVariableDeclarationStatement)e);
			return;
		}
		if (e is CodeAttachEventStatement)
		{
			ValidateAttachEventStatement((CodeAttachEventStatement)e);
			return;
		}
		if (e is CodeRemoveEventStatement)
		{
			ValidateRemoveEventStatement((CodeRemoveEventStatement)e);
			return;
		}
		if (e is CodeGotoStatement)
		{
			ValidateGotoStatement((CodeGotoStatement)e);
			return;
		}
		if (e is CodeLabeledStatement)
		{
			ValidateLabeledStatement((CodeLabeledStatement)e);
			return;
		}
		throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
	}

	private void ValidateStatements(CodeStatementCollection stmts)
	{
		foreach (CodeStatement stmt in stmts)
		{
			ValidateStatement(stmt);
		}
	}

	private void ValidateExpressionStatement(CodeExpressionStatement e)
	{
		ValidateExpression(e.Expression);
	}

	private void ValidateIterationStatement(CodeIterationStatement e)
	{
		ValidateStatement(e.InitStatement);
		ValidateExpression(e.TestExpression);
		ValidateStatement(e.IncrementStatement);
		ValidateStatements(e.Statements);
	}

	private void ValidateThrowExceptionStatement(CodeThrowExceptionStatement e)
	{
		if (e.ToThrow != null)
		{
			ValidateExpression(e.ToThrow);
		}
	}

	private void ValidateMethodReturnStatement(CodeMethodReturnStatement e)
	{
		if (e.Expression != null)
		{
			ValidateExpression(e.Expression);
		}
	}

	private void ValidateConditionStatement(CodeConditionStatement e)
	{
		ValidateExpression(e.Condition);
		ValidateStatements(e.TrueStatements);
		if (e.FalseStatements.Count > 0)
		{
			ValidateStatements(e.FalseStatements);
		}
	}

	private void ValidateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e)
	{
		ValidateStatements(e.TryStatements);
		CodeCatchClauseCollection catchClauses = e.CatchClauses;
		if (catchClauses.Count > 0)
		{
			foreach (CodeCatchClause item in catchClauses)
			{
				ValidateTypeReference(item.CatchExceptionType);
				ValidateIdentifier(item, "LocalName", item.LocalName);
				ValidateStatements(item.Statements);
			}
		}
		CodeStatementCollection finallyStatements = e.FinallyStatements;
		if (finallyStatements.Count > 0)
		{
			ValidateStatements(finallyStatements);
		}
	}

	private void ValidateAssignStatement(CodeAssignStatement e)
	{
		ValidateExpression(e.Left);
		ValidateExpression(e.Right);
	}

	private void ValidateAttachEventStatement(CodeAttachEventStatement e)
	{
		ValidateEventReferenceExpression(e.Event);
		ValidateExpression(e.Listener);
	}

	private void ValidateRemoveEventStatement(CodeRemoveEventStatement e)
	{
		ValidateEventReferenceExpression(e.Event);
		ValidateExpression(e.Listener);
	}

	private static void ValidateGotoStatement(CodeGotoStatement e)
	{
		ValidateIdentifier(e, "Label", e.Label);
	}

	private void ValidateLabeledStatement(CodeLabeledStatement e)
	{
		ValidateIdentifier(e, "Label", e.Label);
		if (e.Statement != null)
		{
			ValidateStatement(e.Statement);
		}
	}

	private void ValidateVariableDeclarationStatement(CodeVariableDeclarationStatement e)
	{
		ValidateTypeReference(e.Type);
		ValidateIdentifier(e, "Name", e.Name);
		if (e.InitExpression != null)
		{
			ValidateExpression(e.InitExpression);
		}
	}

	private void ValidateLinePragmaStart(CodeLinePragma e)
	{
	}

	private void ValidateEvent(CodeMemberEvent e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		if (e.PrivateImplementationType != null)
		{
			ValidateTypeReference(e.Type);
			ValidateIdentifier(e, "Name", e.Name);
		}
		ValidateTypeReferences(e.ImplementationTypes);
	}

	private void ValidateParameters(CodeParameterDeclarationExpressionCollection parameters)
	{
		foreach (CodeParameterDeclarationExpression parameter in parameters)
		{
			ValidateParameterDeclarationExpression(parameter);
		}
	}

	private void ValidateSnippetStatement(CodeSnippetStatement e)
	{
	}

	private void ValidateExpressionList(CodeExpressionCollection expressions)
	{
		foreach (CodeExpression expression in expressions)
		{
			ValidateExpression(expression);
		}
	}

	private static void ValidateTypeReference(CodeTypeReference e)
	{
		ValidateTypeName(e, "BaseType", e.BaseType);
		ValidateArity(e);
		ValidateTypeReferences(e.TypeArguments);
	}

	private static void ValidateTypeReferences(CodeTypeReferenceCollection refs)
	{
		for (int i = 0; i < refs.Count; i++)
		{
			ValidateTypeReference(refs[i]);
		}
	}

	private static void ValidateArity(CodeTypeReference e)
	{
		string baseType = e.BaseType;
		int num = 0;
		for (int i = 0; i < baseType.Length; i++)
		{
			if (baseType[i] == '`')
			{
				i++;
				int num2 = 0;
				for (; i < baseType.Length && baseType[i] >= '0' && baseType[i] <= '9'; i++)
				{
					num2 = num2 * 10 + (baseType[i] - 48);
				}
				num += num2;
			}
		}
		if (num != e.TypeArguments.Count && e.TypeArguments.Count != 0)
		{
			throw new ArgumentException(global::SR.Format("The total arity specified in '{0}' does not match the number of TypeArguments supplied.  There were '{1}' TypeArguments supplied.", baseType, e.TypeArguments.Count));
		}
	}

	private static void ValidateTypeName(object e, string propertyName, string typeName)
	{
		if (!CodeGenerator.IsValidLanguageIndependentTypeName(typeName))
		{
			throw new ArgumentException(global::SR.Format("The type name:\"{0}\" on the property:\"{1}\" of type:\"{2}\" is not a valid language-independent type name.", typeName, propertyName, e.GetType().FullName), "typeName");
		}
	}

	private static void ValidateIdentifier(object e, string propertyName, string identifier)
	{
		if (!CodeGenerator.IsValidLanguageIndependentIdentifier(identifier))
		{
			throw new ArgumentException(global::SR.Format("The identifier:\"{0}\" on the property:\"{1}\" of type:\"{2}\" is not a valid language-independent identifier name. Check to see if CodeGenerator.IsValidLanguageIndependentIdentifier allows the identifier name.", identifier, propertyName, e.GetType().FullName), "identifier");
		}
	}

	private void ValidateExpression(CodeExpression e)
	{
		if (e is CodeArrayCreateExpression)
		{
			ValidateArrayCreateExpression((CodeArrayCreateExpression)e);
			return;
		}
		if (e is CodeBaseReferenceExpression)
		{
			ValidateBaseReferenceExpression((CodeBaseReferenceExpression)e);
			return;
		}
		if (e is CodeBinaryOperatorExpression)
		{
			ValidateBinaryOperatorExpression((CodeBinaryOperatorExpression)e);
			return;
		}
		if (e is CodeCastExpression)
		{
			ValidateCastExpression((CodeCastExpression)e);
			return;
		}
		if (e is CodeDefaultValueExpression)
		{
			ValidateDefaultValueExpression((CodeDefaultValueExpression)e);
			return;
		}
		if (e is CodeDelegateCreateExpression)
		{
			ValidateDelegateCreateExpression((CodeDelegateCreateExpression)e);
			return;
		}
		if (e is CodeFieldReferenceExpression)
		{
			ValidateFieldReferenceExpression((CodeFieldReferenceExpression)e);
			return;
		}
		if (e is CodeArgumentReferenceExpression)
		{
			ValidateArgumentReferenceExpression((CodeArgumentReferenceExpression)e);
			return;
		}
		if (e is CodeVariableReferenceExpression)
		{
			ValidateVariableReferenceExpression((CodeVariableReferenceExpression)e);
			return;
		}
		if (e is CodeIndexerExpression)
		{
			ValidateIndexerExpression((CodeIndexerExpression)e);
			return;
		}
		if (e is CodeArrayIndexerExpression)
		{
			ValidateArrayIndexerExpression((CodeArrayIndexerExpression)e);
			return;
		}
		if (e is CodeSnippetExpression)
		{
			ValidateSnippetExpression((CodeSnippetExpression)e);
			return;
		}
		if (e is CodeMethodInvokeExpression)
		{
			ValidateMethodInvokeExpression((CodeMethodInvokeExpression)e);
			return;
		}
		if (e is CodeMethodReferenceExpression)
		{
			ValidateMethodReferenceExpression((CodeMethodReferenceExpression)e);
			return;
		}
		if (e is CodeEventReferenceExpression)
		{
			ValidateEventReferenceExpression((CodeEventReferenceExpression)e);
			return;
		}
		if (e is CodeDelegateInvokeExpression)
		{
			ValidateDelegateInvokeExpression((CodeDelegateInvokeExpression)e);
			return;
		}
		if (e is CodeObjectCreateExpression)
		{
			ValidateObjectCreateExpression((CodeObjectCreateExpression)e);
			return;
		}
		if (e is CodeParameterDeclarationExpression)
		{
			ValidateParameterDeclarationExpression((CodeParameterDeclarationExpression)e);
			return;
		}
		if (e is CodeDirectionExpression)
		{
			ValidateDirectionExpression((CodeDirectionExpression)e);
			return;
		}
		if (e is CodePrimitiveExpression)
		{
			ValidatePrimitiveExpression((CodePrimitiveExpression)e);
			return;
		}
		if (e is CodePropertyReferenceExpression)
		{
			ValidatePropertyReferenceExpression((CodePropertyReferenceExpression)e);
			return;
		}
		if (e is CodePropertySetValueReferenceExpression)
		{
			ValidatePropertySetValueReferenceExpression((CodePropertySetValueReferenceExpression)e);
			return;
		}
		if (e is CodeThisReferenceExpression)
		{
			ValidateThisReferenceExpression((CodeThisReferenceExpression)e);
			return;
		}
		if (e is CodeTypeReferenceExpression)
		{
			ValidateTypeReference(((CodeTypeReferenceExpression)e).Type);
			return;
		}
		if (e is CodeTypeOfExpression)
		{
			ValidateTypeOfExpression((CodeTypeOfExpression)e);
			return;
		}
		if (e == null)
		{
			throw new ArgumentNullException("e");
		}
		throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
	}

	private void ValidateArrayCreateExpression(CodeArrayCreateExpression e)
	{
		ValidateTypeReference(e.CreateType);
		CodeExpressionCollection initializers = e.Initializers;
		if (initializers.Count > 0)
		{
			ValidateExpressionList(initializers);
		}
		else if (e.SizeExpression != null)
		{
			ValidateExpression(e.SizeExpression);
		}
	}

	private void ValidateBaseReferenceExpression(CodeBaseReferenceExpression e)
	{
	}

	private void ValidateBinaryOperatorExpression(CodeBinaryOperatorExpression e)
	{
		ValidateExpression(e.Left);
		ValidateExpression(e.Right);
	}

	private void ValidateCastExpression(CodeCastExpression e)
	{
		ValidateTypeReference(e.TargetType);
		ValidateExpression(e.Expression);
	}

	private static void ValidateDefaultValueExpression(CodeDefaultValueExpression e)
	{
		ValidateTypeReference(e.Type);
	}

	private void ValidateDelegateCreateExpression(CodeDelegateCreateExpression e)
	{
		ValidateTypeReference(e.DelegateType);
		ValidateExpression(e.TargetObject);
		ValidateIdentifier(e, "MethodName", e.MethodName);
	}

	private void ValidateFieldReferenceExpression(CodeFieldReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			ValidateExpression(e.TargetObject);
		}
		ValidateIdentifier(e, "FieldName", e.FieldName);
	}

	private static void ValidateArgumentReferenceExpression(CodeArgumentReferenceExpression e)
	{
		ValidateIdentifier(e, "ParameterName", e.ParameterName);
	}

	private static void ValidateVariableReferenceExpression(CodeVariableReferenceExpression e)
	{
		ValidateIdentifier(e, "VariableName", e.VariableName);
	}

	private void ValidateIndexerExpression(CodeIndexerExpression e)
	{
		ValidateExpression(e.TargetObject);
		foreach (CodeExpression index in e.Indices)
		{
			ValidateExpression(index);
		}
	}

	private void ValidateArrayIndexerExpression(CodeArrayIndexerExpression e)
	{
		ValidateExpression(e.TargetObject);
		foreach (CodeExpression index in e.Indices)
		{
			ValidateExpression(index);
		}
	}

	private void ValidateSnippetExpression(CodeSnippetExpression e)
	{
	}

	private void ValidateMethodInvokeExpression(CodeMethodInvokeExpression e)
	{
		ValidateMethodReferenceExpression(e.Method);
		ValidateExpressionList(e.Parameters);
	}

	private void ValidateMethodReferenceExpression(CodeMethodReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			ValidateExpression(e.TargetObject);
		}
		ValidateIdentifier(e, "MethodName", e.MethodName);
		ValidateTypeReferences(e.TypeArguments);
	}

	private void ValidateEventReferenceExpression(CodeEventReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			ValidateExpression(e.TargetObject);
		}
		ValidateIdentifier(e, "EventName", e.EventName);
	}

	private void ValidateDelegateInvokeExpression(CodeDelegateInvokeExpression e)
	{
		if (e.TargetObject != null)
		{
			ValidateExpression(e.TargetObject);
		}
		ValidateExpressionList(e.Parameters);
	}

	private void ValidateObjectCreateExpression(CodeObjectCreateExpression e)
	{
		ValidateTypeReference(e.CreateType);
		ValidateExpressionList(e.Parameters);
	}

	private void ValidateParameterDeclarationExpression(CodeParameterDeclarationExpression e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			ValidateAttributes(e.CustomAttributes);
		}
		ValidateTypeReference(e.Type);
		ValidateIdentifier(e, "Name", e.Name);
	}

	private void ValidateDirectionExpression(CodeDirectionExpression e)
	{
		ValidateExpression(e.Expression);
	}

	private void ValidatePrimitiveExpression(CodePrimitiveExpression e)
	{
	}

	private void ValidatePropertyReferenceExpression(CodePropertyReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			ValidateExpression(e.TargetObject);
		}
		ValidateIdentifier(e, "PropertyName", e.PropertyName);
	}

	private void ValidatePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e)
	{
	}

	private void ValidateThisReferenceExpression(CodeThisReferenceExpression e)
	{
	}

	private static void ValidateTypeOfExpression(CodeTypeOfExpression e)
	{
		ValidateTypeReference(e.Type);
	}

	private static void ValidateCodeDirectives(CodeDirectiveCollection e)
	{
		for (int i = 0; i < e.Count; i++)
		{
			ValidateCodeDirective(e[i]);
		}
	}

	private static void ValidateCodeDirective(CodeDirective e)
	{
		if (e is CodeChecksumPragma)
		{
			ValidateChecksumPragma((CodeChecksumPragma)e);
			return;
		}
		if (e is CodeRegionDirective)
		{
			ValidateRegionDirective((CodeRegionDirective)e);
			return;
		}
		throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
	}

	private static void ValidateChecksumPragma(CodeChecksumPragma e)
	{
		if (e.FileName.IndexOfAny(Path.GetInvalidPathChars()) != -1)
		{
			throw new ArgumentException(global::SR.Format("The CodeChecksumPragma file name '{0}' contains invalid path characters.", e.FileName));
		}
	}

	private static void ValidateRegionDirective(CodeRegionDirective e)
	{
		if (e.RegionText.IndexOfAny(s_newLineChars) != -1)
		{
			throw new ArgumentException(global::SR.Format("The region directive '{0}' contains invalid characters.  RegionText cannot contain any new line characters.", e.RegionText));
		}
	}
}
