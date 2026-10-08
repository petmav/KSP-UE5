using System.Globalization;
using System.IO;
using System.Reflection;

namespace System.CodeDom.Compiler;

public abstract class CodeGenerator : ICodeGenerator
{
	private const int ParameterMultilineThreshold = 15;

	private ExposedTabStringIndentedTextWriter _output;

	private CodeGeneratorOptions _options;

	private CodeTypeDeclaration _currentClass;

	private CodeTypeMember _currentMember;

	private bool _inNestedBinary;

	protected CodeTypeDeclaration CurrentClass => _currentClass;

	protected string CurrentTypeName
	{
		get
		{
			if (_currentClass == null)
			{
				return "<% unknown %>";
			}
			return _currentClass.Name;
		}
	}

	protected CodeTypeMember CurrentMember => _currentMember;

	protected string CurrentMemberName
	{
		get
		{
			if (_currentMember == null)
			{
				return "<% unknown %>";
			}
			return _currentMember.Name;
		}
	}

	protected bool IsCurrentInterface
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

	protected bool IsCurrentClass
	{
		get
		{
			if (_currentClass == null || _currentClass is CodeTypeDelegate)
			{
				return false;
			}
			return _currentClass.IsClass;
		}
	}

	protected bool IsCurrentStruct
	{
		get
		{
			if (_currentClass == null || _currentClass is CodeTypeDelegate)
			{
				return false;
			}
			return _currentClass.IsStruct;
		}
	}

	protected bool IsCurrentEnum
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

	protected bool IsCurrentDelegate
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

	protected int Indent
	{
		get
		{
			return _output.Indent;
		}
		set
		{
			_output.Indent = value;
		}
	}

	protected abstract string NullToken { get; }

	protected TextWriter Output => _output;

	protected CodeGeneratorOptions Options => _options;

	private void GenerateType(CodeTypeDeclaration e)
	{
		_currentClass = e;
		if (e.StartDirectives.Count > 0)
		{
			GenerateDirectives(e.StartDirectives);
		}
		GenerateCommentStatements(e.Comments);
		if (e.LinePragma != null)
		{
			GenerateLinePragmaStart(e.LinePragma);
		}
		GenerateTypeStart(e);
		if (Options.VerbatimOrder)
		{
			foreach (CodeTypeMember member in e.Members)
			{
				GenerateTypeMember(member, e);
			}
		}
		else
		{
			GenerateFields(e);
			GenerateSnippetMembers(e);
			GenerateTypeConstructors(e);
			GenerateConstructors(e);
			GenerateProperties(e);
			GenerateEvents(e);
			GenerateMethods(e);
			GenerateNestedTypes(e);
		}
		_currentClass = e;
		GenerateTypeEnd(e);
		if (e.LinePragma != null)
		{
			GenerateLinePragmaEnd(e.LinePragma);
		}
		if (e.EndDirectives.Count > 0)
		{
			GenerateDirectives(e.EndDirectives);
		}
	}

	protected virtual void GenerateDirectives(CodeDirectiveCollection directives)
	{
	}

	private void GenerateTypeMember(CodeTypeMember member, CodeTypeDeclaration declaredType)
	{
		if (_options.BlankLinesBetweenMembers)
		{
			Output.WriteLine();
		}
		if (member is CodeTypeDeclaration)
		{
			((ICodeGenerator)this).GenerateCodeFromType((CodeTypeDeclaration)member, _output.InnerWriter, _options);
			_currentClass = declaredType;
			return;
		}
		if (member.StartDirectives.Count > 0)
		{
			GenerateDirectives(member.StartDirectives);
		}
		GenerateCommentStatements(member.Comments);
		if (member.LinePragma != null)
		{
			GenerateLinePragmaStart(member.LinePragma);
		}
		if (member is CodeMemberField)
		{
			GenerateField((CodeMemberField)member);
		}
		else if (member is CodeMemberProperty)
		{
			GenerateProperty((CodeMemberProperty)member, declaredType);
		}
		else if (member is CodeMemberMethod)
		{
			if (member is CodeConstructor)
			{
				GenerateConstructor((CodeConstructor)member, declaredType);
			}
			else if (member is CodeTypeConstructor)
			{
				GenerateTypeConstructor((CodeTypeConstructor)member);
			}
			else if (member is CodeEntryPointMethod)
			{
				GenerateEntryPointMethod((CodeEntryPointMethod)member, declaredType);
			}
			else
			{
				GenerateMethod((CodeMemberMethod)member, declaredType);
			}
		}
		else if (member is CodeMemberEvent)
		{
			GenerateEvent((CodeMemberEvent)member, declaredType);
		}
		else if (member is CodeSnippetTypeMember)
		{
			int indent = Indent;
			Indent = 0;
			GenerateSnippetMember((CodeSnippetTypeMember)member);
			Indent = indent;
			Output.WriteLine();
		}
		if (member.LinePragma != null)
		{
			GenerateLinePragmaEnd(member.LinePragma);
		}
		if (member.EndDirectives.Count > 0)
		{
			GenerateDirectives(member.EndDirectives);
		}
	}

	private void GenerateTypeConstructors(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeTypeConstructor)
			{
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeTypeConstructor codeTypeConstructor = (CodeTypeConstructor)member;
				if (codeTypeConstructor.LinePragma != null)
				{
					GenerateLinePragmaStart(codeTypeConstructor.LinePragma);
				}
				GenerateTypeConstructor(codeTypeConstructor);
				if (codeTypeConstructor.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeTypeConstructor.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
	}

	protected void GenerateNamespaces(CodeCompileUnit e)
	{
		foreach (CodeNamespace @namespace in e.Namespaces)
		{
			((ICodeGenerator)this).GenerateCodeFromNamespace(@namespace, _output.InnerWriter, _options);
		}
	}

	protected void GenerateTypes(CodeNamespace e)
	{
		foreach (CodeTypeDeclaration type in e.Types)
		{
			if (_options.BlankLinesBetweenMembers)
			{
				Output.WriteLine();
			}
			((ICodeGenerator)this).GenerateCodeFromType(type, _output.InnerWriter, _options);
		}
	}

	bool ICodeGenerator.Supports(GeneratorSupport support)
	{
		return Supports(support);
	}

	void ICodeGenerator.GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o)
	{
		bool flag = false;
		if (_output != null && w != _output.InnerWriter)
		{
			throw new InvalidOperationException("The output writer for code generation and the writer supplied don't match and cannot be used. This is generally caused by a bad implementation of a CodeGenerator derived class.");
		}
		if (_output == null)
		{
			flag = true;
			_options = o ?? new CodeGeneratorOptions();
			_output = new ExposedTabStringIndentedTextWriter(w, _options.IndentString);
		}
		try
		{
			GenerateType(e);
		}
		finally
		{
			if (flag)
			{
				_output = null;
				_options = null;
			}
		}
	}

	void ICodeGenerator.GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o)
	{
		bool flag = false;
		if (_output != null && w != _output.InnerWriter)
		{
			throw new InvalidOperationException("The output writer for code generation and the writer supplied don't match and cannot be used. This is generally caused by a bad implementation of a CodeGenerator derived class.");
		}
		if (_output == null)
		{
			flag = true;
			_options = o ?? new CodeGeneratorOptions();
			_output = new ExposedTabStringIndentedTextWriter(w, _options.IndentString);
		}
		try
		{
			GenerateExpression(e);
		}
		finally
		{
			if (flag)
			{
				_output = null;
				_options = null;
			}
		}
	}

	void ICodeGenerator.GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o)
	{
		bool flag = false;
		if (_output != null && w != _output.InnerWriter)
		{
			throw new InvalidOperationException("The output writer for code generation and the writer supplied don't match and cannot be used. This is generally caused by a bad implementation of a CodeGenerator derived class.");
		}
		if (_output == null)
		{
			flag = true;
			_options = o ?? new CodeGeneratorOptions();
			_output = new ExposedTabStringIndentedTextWriter(w, _options.IndentString);
		}
		try
		{
			if (e is CodeSnippetCompileUnit)
			{
				GenerateSnippetCompileUnit((CodeSnippetCompileUnit)e);
			}
			else
			{
				GenerateCompileUnit(e);
			}
		}
		finally
		{
			if (flag)
			{
				_output = null;
				_options = null;
			}
		}
	}

	void ICodeGenerator.GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o)
	{
		bool flag = false;
		if (_output != null && w != _output.InnerWriter)
		{
			throw new InvalidOperationException("The output writer for code generation and the writer supplied don't match and cannot be used. This is generally caused by a bad implementation of a CodeGenerator derived class.");
		}
		if (_output == null)
		{
			flag = true;
			_options = o ?? new CodeGeneratorOptions();
			_output = new ExposedTabStringIndentedTextWriter(w, _options.IndentString);
		}
		try
		{
			GenerateNamespace(e);
		}
		finally
		{
			if (flag)
			{
				_output = null;
				_options = null;
			}
		}
	}

	void ICodeGenerator.GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o)
	{
		bool flag = false;
		if (_output != null && w != _output.InnerWriter)
		{
			throw new InvalidOperationException("The output writer for code generation and the writer supplied don't match and cannot be used. This is generally caused by a bad implementation of a CodeGenerator derived class.");
		}
		if (_output == null)
		{
			flag = true;
			_options = o ?? new CodeGeneratorOptions();
			_output = new ExposedTabStringIndentedTextWriter(w, _options.IndentString);
		}
		try
		{
			GenerateStatement(e);
		}
		finally
		{
			if (flag)
			{
				_output = null;
				_options = null;
			}
		}
	}

	public virtual void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
	{
		if (_output != null)
		{
			throw new InvalidOperationException("This code generation API cannot be called while the generator is being used to generate something else.");
		}
		_options = options ?? new CodeGeneratorOptions();
		_output = new ExposedTabStringIndentedTextWriter(writer, _options.IndentString);
		try
		{
			GenerateTypeMember(member, _currentClass = new CodeTypeDeclaration());
		}
		finally
		{
			_currentClass = null;
			_output = null;
			_options = null;
		}
	}

	bool ICodeGenerator.IsValidIdentifier(string value)
	{
		return IsValidIdentifier(value);
	}

	void ICodeGenerator.ValidateIdentifier(string value)
	{
		ValidateIdentifier(value);
	}

	string ICodeGenerator.CreateEscapedIdentifier(string value)
	{
		return CreateEscapedIdentifier(value);
	}

	string ICodeGenerator.CreateValidIdentifier(string value)
	{
		return CreateValidIdentifier(value);
	}

	string ICodeGenerator.GetTypeOutput(CodeTypeReference type)
	{
		return GetTypeOutput(type);
	}

	private void GenerateConstructors(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeConstructor)
			{
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeConstructor codeConstructor = (CodeConstructor)member;
				if (codeConstructor.LinePragma != null)
				{
					GenerateLinePragmaStart(codeConstructor.LinePragma);
				}
				GenerateConstructor(codeConstructor, e);
				if (codeConstructor.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeConstructor.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
	}

	private void GenerateEvents(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeMemberEvent)
			{
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeMemberEvent codeMemberEvent = (CodeMemberEvent)member;
				if (codeMemberEvent.LinePragma != null)
				{
					GenerateLinePragmaStart(codeMemberEvent.LinePragma);
				}
				GenerateEvent(codeMemberEvent, e);
				if (codeMemberEvent.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeMemberEvent.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
	}

	protected void GenerateExpression(CodeExpression e)
	{
		if (e is CodeArrayCreateExpression)
		{
			GenerateArrayCreateExpression((CodeArrayCreateExpression)e);
			return;
		}
		if (e is CodeBaseReferenceExpression)
		{
			GenerateBaseReferenceExpression((CodeBaseReferenceExpression)e);
			return;
		}
		if (e is CodeBinaryOperatorExpression)
		{
			GenerateBinaryOperatorExpression((CodeBinaryOperatorExpression)e);
			return;
		}
		if (e is CodeCastExpression)
		{
			GenerateCastExpression((CodeCastExpression)e);
			return;
		}
		if (e is CodeDelegateCreateExpression)
		{
			GenerateDelegateCreateExpression((CodeDelegateCreateExpression)e);
			return;
		}
		if (e is CodeFieldReferenceExpression)
		{
			GenerateFieldReferenceExpression((CodeFieldReferenceExpression)e);
			return;
		}
		if (e is CodeArgumentReferenceExpression)
		{
			GenerateArgumentReferenceExpression((CodeArgumentReferenceExpression)e);
			return;
		}
		if (e is CodeVariableReferenceExpression)
		{
			GenerateVariableReferenceExpression((CodeVariableReferenceExpression)e);
			return;
		}
		if (e is CodeIndexerExpression)
		{
			GenerateIndexerExpression((CodeIndexerExpression)e);
			return;
		}
		if (e is CodeArrayIndexerExpression)
		{
			GenerateArrayIndexerExpression((CodeArrayIndexerExpression)e);
			return;
		}
		if (e is CodeSnippetExpression)
		{
			GenerateSnippetExpression((CodeSnippetExpression)e);
			return;
		}
		if (e is CodeMethodInvokeExpression)
		{
			GenerateMethodInvokeExpression((CodeMethodInvokeExpression)e);
			return;
		}
		if (e is CodeMethodReferenceExpression)
		{
			GenerateMethodReferenceExpression((CodeMethodReferenceExpression)e);
			return;
		}
		if (e is CodeEventReferenceExpression)
		{
			GenerateEventReferenceExpression((CodeEventReferenceExpression)e);
			return;
		}
		if (e is CodeDelegateInvokeExpression)
		{
			GenerateDelegateInvokeExpression((CodeDelegateInvokeExpression)e);
			return;
		}
		if (e is CodeObjectCreateExpression)
		{
			GenerateObjectCreateExpression((CodeObjectCreateExpression)e);
			return;
		}
		if (e is CodeParameterDeclarationExpression)
		{
			GenerateParameterDeclarationExpression((CodeParameterDeclarationExpression)e);
			return;
		}
		if (e is CodeDirectionExpression)
		{
			GenerateDirectionExpression((CodeDirectionExpression)e);
			return;
		}
		if (e is CodePrimitiveExpression)
		{
			GeneratePrimitiveExpression((CodePrimitiveExpression)e);
			return;
		}
		if (e is CodePropertyReferenceExpression)
		{
			GeneratePropertyReferenceExpression((CodePropertyReferenceExpression)e);
			return;
		}
		if (e is CodePropertySetValueReferenceExpression)
		{
			GeneratePropertySetValueReferenceExpression((CodePropertySetValueReferenceExpression)e);
			return;
		}
		if (e is CodeThisReferenceExpression)
		{
			GenerateThisReferenceExpression((CodeThisReferenceExpression)e);
			return;
		}
		if (e is CodeTypeReferenceExpression)
		{
			GenerateTypeReferenceExpression((CodeTypeReferenceExpression)e);
			return;
		}
		if (e is CodeTypeOfExpression)
		{
			GenerateTypeOfExpression((CodeTypeOfExpression)e);
			return;
		}
		if (e is CodeDefaultValueExpression)
		{
			GenerateDefaultValueExpression((CodeDefaultValueExpression)e);
			return;
		}
		if (e == null)
		{
			throw new ArgumentNullException("e");
		}
		throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
	}

	private void GenerateFields(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeMemberField)
			{
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeMemberField codeMemberField = (CodeMemberField)member;
				if (codeMemberField.LinePragma != null)
				{
					GenerateLinePragmaStart(codeMemberField.LinePragma);
				}
				GenerateField(codeMemberField);
				if (codeMemberField.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeMemberField.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
	}

	private void GenerateSnippetMembers(CodeTypeDeclaration e)
	{
		bool flag = false;
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeSnippetTypeMember)
			{
				flag = true;
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeSnippetTypeMember codeSnippetTypeMember = (CodeSnippetTypeMember)member;
				if (codeSnippetTypeMember.LinePragma != null)
				{
					GenerateLinePragmaStart(codeSnippetTypeMember.LinePragma);
				}
				int indent = Indent;
				Indent = 0;
				GenerateSnippetMember(codeSnippetTypeMember);
				Indent = indent;
				if (codeSnippetTypeMember.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeSnippetTypeMember.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
		if (flag)
		{
			Output.WriteLine();
		}
	}

	protected virtual void GenerateSnippetCompileUnit(CodeSnippetCompileUnit e)
	{
		GenerateDirectives(e.StartDirectives);
		if (e.LinePragma != null)
		{
			GenerateLinePragmaStart(e.LinePragma);
		}
		Output.WriteLine(e.Value);
		if (e.LinePragma != null)
		{
			GenerateLinePragmaEnd(e.LinePragma);
		}
		if (e.EndDirectives.Count > 0)
		{
			GenerateDirectives(e.EndDirectives);
		}
	}

	private void GenerateMethods(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeMemberMethod && !(member is CodeTypeConstructor) && !(member is CodeConstructor))
			{
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeMemberMethod codeMemberMethod = (CodeMemberMethod)member;
				if (codeMemberMethod.LinePragma != null)
				{
					GenerateLinePragmaStart(codeMemberMethod.LinePragma);
				}
				if (member is CodeEntryPointMethod)
				{
					GenerateEntryPointMethod((CodeEntryPointMethod)member, e);
				}
				else
				{
					GenerateMethod(codeMemberMethod, e);
				}
				if (codeMemberMethod.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeMemberMethod.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
	}

	private void GenerateNestedTypes(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeTypeDeclaration)
			{
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				CodeTypeDeclaration e2 = (CodeTypeDeclaration)member;
				((ICodeGenerator)this).GenerateCodeFromType(e2, _output.InnerWriter, _options);
			}
		}
	}

	protected virtual void GenerateCompileUnit(CodeCompileUnit e)
	{
		GenerateCompileUnitStart(e);
		GenerateNamespaces(e);
		GenerateCompileUnitEnd(e);
	}

	protected virtual void GenerateNamespace(CodeNamespace e)
	{
		GenerateCommentStatements(e.Comments);
		GenerateNamespaceStart(e);
		GenerateNamespaceImports(e);
		Output.WriteLine();
		GenerateTypes(e);
		GenerateNamespaceEnd(e);
	}

	protected void GenerateNamespaceImports(CodeNamespace e)
	{
		foreach (CodeNamespaceImport import in e.Imports)
		{
			if (import.LinePragma != null)
			{
				GenerateLinePragmaStart(import.LinePragma);
			}
			GenerateNamespaceImport(import);
			if (import.LinePragma != null)
			{
				GenerateLinePragmaEnd(import.LinePragma);
			}
		}
	}

	private void GenerateProperties(CodeTypeDeclaration e)
	{
		foreach (CodeTypeMember member in e.Members)
		{
			if (member is CodeMemberProperty)
			{
				_currentMember = member;
				if (_options.BlankLinesBetweenMembers)
				{
					Output.WriteLine();
				}
				if (_currentMember.StartDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.StartDirectives);
				}
				GenerateCommentStatements(_currentMember.Comments);
				CodeMemberProperty codeMemberProperty = (CodeMemberProperty)member;
				if (codeMemberProperty.LinePragma != null)
				{
					GenerateLinePragmaStart(codeMemberProperty.LinePragma);
				}
				GenerateProperty(codeMemberProperty, e);
				if (codeMemberProperty.LinePragma != null)
				{
					GenerateLinePragmaEnd(codeMemberProperty.LinePragma);
				}
				if (_currentMember.EndDirectives.Count > 0)
				{
					GenerateDirectives(_currentMember.EndDirectives);
				}
			}
		}
	}

	protected void GenerateStatement(CodeStatement e)
	{
		if (e.StartDirectives.Count > 0)
		{
			GenerateDirectives(e.StartDirectives);
		}
		if (e.LinePragma != null)
		{
			GenerateLinePragmaStart(e.LinePragma);
		}
		if (e is CodeCommentStatement)
		{
			GenerateCommentStatement((CodeCommentStatement)e);
		}
		else if (e is CodeMethodReturnStatement)
		{
			GenerateMethodReturnStatement((CodeMethodReturnStatement)e);
		}
		else if (e is CodeConditionStatement)
		{
			GenerateConditionStatement((CodeConditionStatement)e);
		}
		else if (e is CodeTryCatchFinallyStatement)
		{
			GenerateTryCatchFinallyStatement((CodeTryCatchFinallyStatement)e);
		}
		else if (e is CodeAssignStatement)
		{
			GenerateAssignStatement((CodeAssignStatement)e);
		}
		else if (e is CodeExpressionStatement)
		{
			GenerateExpressionStatement((CodeExpressionStatement)e);
		}
		else if (e is CodeIterationStatement)
		{
			GenerateIterationStatement((CodeIterationStatement)e);
		}
		else if (e is CodeThrowExceptionStatement)
		{
			GenerateThrowExceptionStatement((CodeThrowExceptionStatement)e);
		}
		else if (e is CodeSnippetStatement)
		{
			int indent = Indent;
			Indent = 0;
			GenerateSnippetStatement((CodeSnippetStatement)e);
			Indent = indent;
		}
		else if (e is CodeVariableDeclarationStatement)
		{
			GenerateVariableDeclarationStatement((CodeVariableDeclarationStatement)e);
		}
		else if (e is CodeAttachEventStatement)
		{
			GenerateAttachEventStatement((CodeAttachEventStatement)e);
		}
		else if (e is CodeRemoveEventStatement)
		{
			GenerateRemoveEventStatement((CodeRemoveEventStatement)e);
		}
		else if (e is CodeGotoStatement)
		{
			GenerateGotoStatement((CodeGotoStatement)e);
		}
		else
		{
			if (!(e is CodeLabeledStatement))
			{
				throw new ArgumentException(global::SR.Format("Element type {0} is not supported.", e.GetType().FullName), "e");
			}
			GenerateLabeledStatement((CodeLabeledStatement)e);
		}
		if (e.LinePragma != null)
		{
			GenerateLinePragmaEnd(e.LinePragma);
		}
		if (e.EndDirectives.Count > 0)
		{
			GenerateDirectives(e.EndDirectives);
		}
	}

	protected void GenerateStatements(CodeStatementCollection stmts)
	{
		foreach (CodeStatement stmt in stmts)
		{
			((ICodeGenerator)this).GenerateCodeFromStatement(stmt, _output.InnerWriter, _options);
		}
	}

	protected virtual void OutputAttributeDeclarations(CodeAttributeDeclarationCollection attributes)
	{
		if (attributes.Count == 0)
		{
			return;
		}
		GenerateAttributeDeclarationsStart(attributes);
		bool flag = true;
		foreach (CodeAttributeDeclaration attribute in attributes)
		{
			if (flag)
			{
				flag = false;
			}
			else
			{
				ContinueOnNewLine(", ");
			}
			Output.Write(attribute.Name);
			Output.Write('(');
			bool flag2 = true;
			foreach (CodeAttributeArgument argument in attribute.Arguments)
			{
				if (flag2)
				{
					flag2 = false;
				}
				else
				{
					Output.Write(", ");
				}
				OutputAttributeArgument(argument);
			}
			Output.Write(')');
		}
		GenerateAttributeDeclarationsEnd(attributes);
	}

	protected virtual void OutputAttributeArgument(CodeAttributeArgument arg)
	{
		if (!string.IsNullOrEmpty(arg.Name))
		{
			OutputIdentifier(arg.Name);
			Output.Write('=');
		}
		((ICodeGenerator)this).GenerateCodeFromExpression(arg.Value, _output.InnerWriter, _options);
	}

	protected virtual void OutputDirection(FieldDirection dir)
	{
		switch (dir)
		{
		case FieldDirection.Out:
			Output.Write("out ");
			break;
		case FieldDirection.Ref:
			Output.Write("ref ");
			break;
		case FieldDirection.In:
			break;
		}
	}

	protected virtual void OutputFieldScopeModifier(MemberAttributes attributes)
	{
		MemberAttributes memberAttributes = attributes & MemberAttributes.VTableMask;
		if (memberAttributes == MemberAttributes.New)
		{
			Output.Write("new ");
		}
		switch (attributes & MemberAttributes.ScopeMask)
		{
		case MemberAttributes.Static:
			Output.Write("static ");
			break;
		case MemberAttributes.Const:
			Output.Write("const ");
			break;
		case MemberAttributes.Final:
		case MemberAttributes.Override:
			break;
		}
	}

	protected virtual void OutputMemberAccessModifier(MemberAttributes attributes)
	{
		switch (attributes & MemberAttributes.AccessMask)
		{
		case MemberAttributes.Assembly:
			Output.Write("internal ");
			break;
		case MemberAttributes.FamilyAndAssembly:
			Output.Write("internal ");
			break;
		case MemberAttributes.Family:
			Output.Write("protected ");
			break;
		case MemberAttributes.FamilyOrAssembly:
			Output.Write("protected internal ");
			break;
		case MemberAttributes.Private:
			Output.Write("private ");
			break;
		case MemberAttributes.Public:
			Output.Write("public ");
			break;
		}
	}

	protected virtual void OutputMemberScopeModifier(MemberAttributes attributes)
	{
		MemberAttributes memberAttributes = attributes & MemberAttributes.VTableMask;
		if (memberAttributes == MemberAttributes.New)
		{
			Output.Write("new ");
		}
		switch (attributes & MemberAttributes.ScopeMask)
		{
		case MemberAttributes.Abstract:
			Output.Write("abstract ");
			return;
		case MemberAttributes.Final:
			Output.Write("");
			return;
		case MemberAttributes.Static:
			Output.Write("static ");
			return;
		case MemberAttributes.Override:
			Output.Write("override ");
			return;
		}
		memberAttributes = attributes & MemberAttributes.AccessMask;
		if (memberAttributes == MemberAttributes.Family || memberAttributes == MemberAttributes.Public)
		{
			Output.Write("virtual ");
		}
	}

	protected abstract void OutputType(CodeTypeReference typeRef);

	protected virtual void OutputTypeAttributes(TypeAttributes attributes, bool isStruct, bool isEnum)
	{
		switch (attributes & TypeAttributes.VisibilityMask)
		{
		case TypeAttributes.Public:
		case TypeAttributes.NestedPublic:
			Output.Write("public ");
			break;
		case TypeAttributes.NestedPrivate:
			Output.Write("private ");
			break;
		}
		if (isStruct)
		{
			Output.Write("struct ");
			return;
		}
		if (isEnum)
		{
			Output.Write("enum ");
			return;
		}
		switch (attributes & TypeAttributes.ClassSemanticsMask)
		{
		case TypeAttributes.NotPublic:
			if ((attributes & TypeAttributes.Sealed) == TypeAttributes.Sealed)
			{
				Output.Write("sealed ");
			}
			if ((attributes & TypeAttributes.Abstract) == TypeAttributes.Abstract)
			{
				Output.Write("abstract ");
			}
			Output.Write("class ");
			break;
		case TypeAttributes.ClassSemanticsMask:
			Output.Write("interface ");
			break;
		}
	}

	protected virtual void OutputTypeNamePair(CodeTypeReference typeRef, string name)
	{
		OutputType(typeRef);
		Output.Write(' ');
		OutputIdentifier(name);
	}

	protected virtual void OutputIdentifier(string ident)
	{
		Output.Write(ident);
	}

	protected virtual void OutputExpressionList(CodeExpressionCollection expressions)
	{
		OutputExpressionList(expressions, newlineBetweenItems: false);
	}

	protected virtual void OutputExpressionList(CodeExpressionCollection expressions, bool newlineBetweenItems)
	{
		bool flag = true;
		Indent++;
		foreach (CodeExpression expression in expressions)
		{
			if (flag)
			{
				flag = false;
			}
			else if (newlineBetweenItems)
			{
				ContinueOnNewLine(",");
			}
			else
			{
				Output.Write(", ");
			}
			((ICodeGenerator)this).GenerateCodeFromExpression(expression, _output.InnerWriter, _options);
		}
		Indent--;
	}

	protected virtual void OutputOperator(CodeBinaryOperatorType op)
	{
		switch (op)
		{
		case CodeBinaryOperatorType.Add:
			Output.Write('+');
			break;
		case CodeBinaryOperatorType.Subtract:
			Output.Write('-');
			break;
		case CodeBinaryOperatorType.Multiply:
			Output.Write('*');
			break;
		case CodeBinaryOperatorType.Divide:
			Output.Write('/');
			break;
		case CodeBinaryOperatorType.Modulus:
			Output.Write('%');
			break;
		case CodeBinaryOperatorType.Assign:
			Output.Write('=');
			break;
		case CodeBinaryOperatorType.IdentityInequality:
			Output.Write("!=");
			break;
		case CodeBinaryOperatorType.IdentityEquality:
			Output.Write("==");
			break;
		case CodeBinaryOperatorType.ValueEquality:
			Output.Write("==");
			break;
		case CodeBinaryOperatorType.BitwiseOr:
			Output.Write('|');
			break;
		case CodeBinaryOperatorType.BitwiseAnd:
			Output.Write('&');
			break;
		case CodeBinaryOperatorType.BooleanOr:
			Output.Write("||");
			break;
		case CodeBinaryOperatorType.BooleanAnd:
			Output.Write("&&");
			break;
		case CodeBinaryOperatorType.LessThan:
			Output.Write('<');
			break;
		case CodeBinaryOperatorType.LessThanOrEqual:
			Output.Write("<=");
			break;
		case CodeBinaryOperatorType.GreaterThan:
			Output.Write('>');
			break;
		case CodeBinaryOperatorType.GreaterThanOrEqual:
			Output.Write(">=");
			break;
		}
	}

	protected virtual void OutputParameters(CodeParameterDeclarationExpressionCollection parameters)
	{
		bool flag = true;
		bool flag2 = parameters.Count > 15;
		if (flag2)
		{
			Indent += 3;
		}
		foreach (CodeParameterDeclarationExpression parameter in parameters)
		{
			if (flag)
			{
				flag = false;
			}
			else
			{
				Output.Write(", ");
			}
			if (flag2)
			{
				ContinueOnNewLine("");
			}
			GenerateExpression(parameter);
		}
		if (flag2)
		{
			Indent -= 3;
		}
	}

	protected abstract void GenerateArrayCreateExpression(CodeArrayCreateExpression e);

	protected abstract void GenerateBaseReferenceExpression(CodeBaseReferenceExpression e);

	protected virtual void GenerateBinaryOperatorExpression(CodeBinaryOperatorExpression e)
	{
		bool flag = false;
		Output.Write('(');
		GenerateExpression(e.Left);
		Output.Write(' ');
		if (e.Left is CodeBinaryOperatorExpression || e.Right is CodeBinaryOperatorExpression)
		{
			if (!_inNestedBinary)
			{
				flag = true;
				_inNestedBinary = true;
				Indent += 3;
			}
			ContinueOnNewLine("");
		}
		OutputOperator(e.Operator);
		Output.Write(' ');
		GenerateExpression(e.Right);
		Output.Write(')');
		if (flag)
		{
			Indent -= 3;
			_inNestedBinary = false;
		}
	}

	protected virtual void ContinueOnNewLine(string st)
	{
		Output.WriteLine(st);
	}

	protected abstract void GenerateCastExpression(CodeCastExpression e);

	protected abstract void GenerateDelegateCreateExpression(CodeDelegateCreateExpression e);

	protected abstract void GenerateFieldReferenceExpression(CodeFieldReferenceExpression e);

	protected abstract void GenerateArgumentReferenceExpression(CodeArgumentReferenceExpression e);

	protected abstract void GenerateVariableReferenceExpression(CodeVariableReferenceExpression e);

	protected abstract void GenerateIndexerExpression(CodeIndexerExpression e);

	protected abstract void GenerateArrayIndexerExpression(CodeArrayIndexerExpression e);

	protected abstract void GenerateSnippetExpression(CodeSnippetExpression e);

	protected abstract void GenerateMethodInvokeExpression(CodeMethodInvokeExpression e);

	protected abstract void GenerateMethodReferenceExpression(CodeMethodReferenceExpression e);

	protected abstract void GenerateEventReferenceExpression(CodeEventReferenceExpression e);

	protected abstract void GenerateDelegateInvokeExpression(CodeDelegateInvokeExpression e);

	protected abstract void GenerateObjectCreateExpression(CodeObjectCreateExpression e);

	protected virtual void GenerateParameterDeclarationExpression(CodeParameterDeclarationExpression e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			OutputAttributeDeclarations(e.CustomAttributes);
			Output.Write(' ');
		}
		OutputDirection(e.Direction);
		OutputTypeNamePair(e.Type, e.Name);
	}

	protected virtual void GenerateDirectionExpression(CodeDirectionExpression e)
	{
		OutputDirection(e.Direction);
		GenerateExpression(e.Expression);
	}

	protected virtual void GeneratePrimitiveExpression(CodePrimitiveExpression e)
	{
		if (e.Value == null)
		{
			Output.Write(NullToken);
			return;
		}
		if (e.Value is string)
		{
			Output.Write(QuoteSnippetString((string)e.Value));
			return;
		}
		if (e.Value is char)
		{
			Output.Write("'" + e.Value.ToString() + "'");
			return;
		}
		if (e.Value is byte)
		{
			Output.Write(((byte)e.Value).ToString(CultureInfo.InvariantCulture));
			return;
		}
		if (e.Value is short)
		{
			Output.Write(((short)e.Value).ToString(CultureInfo.InvariantCulture));
			return;
		}
		if (e.Value is int)
		{
			Output.Write(((int)e.Value).ToString(CultureInfo.InvariantCulture));
			return;
		}
		if (e.Value is long)
		{
			Output.Write(((long)e.Value).ToString(CultureInfo.InvariantCulture));
			return;
		}
		if (e.Value is float)
		{
			GenerateSingleFloatValue((float)e.Value);
			return;
		}
		if (e.Value is double)
		{
			GenerateDoubleValue((double)e.Value);
			return;
		}
		if (e.Value is decimal)
		{
			GenerateDecimalValue((decimal)e.Value);
			return;
		}
		if (e.Value is bool)
		{
			if ((bool)e.Value)
			{
				Output.Write("true");
			}
			else
			{
				Output.Write("false");
			}
			return;
		}
		throw new ArgumentException(global::SR.Format("Invalid Primitive Type: {0}. Consider using CodeObjectCreateExpression.", e.Value.GetType().ToString()));
	}

	protected virtual void GenerateSingleFloatValue(float s)
	{
		Output.Write(s.ToString("R", CultureInfo.InvariantCulture));
	}

	protected virtual void GenerateDoubleValue(double d)
	{
		Output.Write(d.ToString("R", CultureInfo.InvariantCulture));
	}

	protected virtual void GenerateDecimalValue(decimal d)
	{
		Output.Write(d.ToString(CultureInfo.InvariantCulture));
	}

	protected virtual void GenerateDefaultValueExpression(CodeDefaultValueExpression e)
	{
	}

	protected abstract void GeneratePropertyReferenceExpression(CodePropertyReferenceExpression e);

	protected abstract void GeneratePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e);

	protected abstract void GenerateThisReferenceExpression(CodeThisReferenceExpression e);

	protected virtual void GenerateTypeReferenceExpression(CodeTypeReferenceExpression e)
	{
		OutputType(e.Type);
	}

	protected virtual void GenerateTypeOfExpression(CodeTypeOfExpression e)
	{
		Output.Write("typeof(");
		OutputType(e.Type);
		Output.Write(')');
	}

	protected abstract void GenerateExpressionStatement(CodeExpressionStatement e);

	protected abstract void GenerateIterationStatement(CodeIterationStatement e);

	protected abstract void GenerateThrowExceptionStatement(CodeThrowExceptionStatement e);

	protected virtual void GenerateCommentStatement(CodeCommentStatement e)
	{
		if (e.Comment == null)
		{
			throw new ArgumentException(global::SR.Format("The 'Comment' property of the CodeCommentStatement '{0}' cannot be null.", "e"), "e");
		}
		GenerateComment(e.Comment);
	}

	protected virtual void GenerateCommentStatements(CodeCommentStatementCollection e)
	{
		foreach (CodeCommentStatement item in e)
		{
			GenerateCommentStatement(item);
		}
	}

	protected abstract void GenerateComment(CodeComment e);

	protected abstract void GenerateMethodReturnStatement(CodeMethodReturnStatement e);

	protected abstract void GenerateConditionStatement(CodeConditionStatement e);

	protected abstract void GenerateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e);

	protected abstract void GenerateAssignStatement(CodeAssignStatement e);

	protected abstract void GenerateAttachEventStatement(CodeAttachEventStatement e);

	protected abstract void GenerateRemoveEventStatement(CodeRemoveEventStatement e);

	protected abstract void GenerateGotoStatement(CodeGotoStatement e);

	protected abstract void GenerateLabeledStatement(CodeLabeledStatement e);

	protected virtual void GenerateSnippetStatement(CodeSnippetStatement e)
	{
		Output.WriteLine(e.Value);
	}

	protected abstract void GenerateVariableDeclarationStatement(CodeVariableDeclarationStatement e);

	protected abstract void GenerateLinePragmaStart(CodeLinePragma e);

	protected abstract void GenerateLinePragmaEnd(CodeLinePragma e);

	protected abstract void GenerateEvent(CodeMemberEvent e, CodeTypeDeclaration c);

	protected abstract void GenerateField(CodeMemberField e);

	protected abstract void GenerateSnippetMember(CodeSnippetTypeMember e);

	protected abstract void GenerateEntryPointMethod(CodeEntryPointMethod e, CodeTypeDeclaration c);

	protected abstract void GenerateMethod(CodeMemberMethod e, CodeTypeDeclaration c);

	protected abstract void GenerateProperty(CodeMemberProperty e, CodeTypeDeclaration c);

	protected abstract void GenerateConstructor(CodeConstructor e, CodeTypeDeclaration c);

	protected abstract void GenerateTypeConstructor(CodeTypeConstructor e);

	protected abstract void GenerateTypeStart(CodeTypeDeclaration e);

	protected abstract void GenerateTypeEnd(CodeTypeDeclaration e);

	protected virtual void GenerateCompileUnitStart(CodeCompileUnit e)
	{
		if (e.StartDirectives.Count > 0)
		{
			GenerateDirectives(e.StartDirectives);
		}
	}

	protected virtual void GenerateCompileUnitEnd(CodeCompileUnit e)
	{
		if (e.EndDirectives.Count > 0)
		{
			GenerateDirectives(e.EndDirectives);
		}
	}

	protected abstract void GenerateNamespaceStart(CodeNamespace e);

	protected abstract void GenerateNamespaceEnd(CodeNamespace e);

	protected abstract void GenerateNamespaceImport(CodeNamespaceImport e);

	protected abstract void GenerateAttributeDeclarationsStart(CodeAttributeDeclarationCollection attributes);

	protected abstract void GenerateAttributeDeclarationsEnd(CodeAttributeDeclarationCollection attributes);

	protected abstract bool Supports(GeneratorSupport support);

	protected abstract bool IsValidIdentifier(string value);

	protected virtual void ValidateIdentifier(string value)
	{
		if (!IsValidIdentifier(value))
		{
			throw new ArgumentException(global::SR.Format("Identifier '{0}' is not valid.", value));
		}
	}

	protected abstract string CreateEscapedIdentifier(string value);

	protected abstract string CreateValidIdentifier(string value);

	protected abstract string GetTypeOutput(CodeTypeReference value);

	protected abstract string QuoteSnippetString(string value);

	public static bool IsValidLanguageIndependentIdentifier(string value)
	{
		return CSharpHelpers.IsValidTypeNameOrIdentifier(value, isTypeName: false);
	}

	internal static bool IsValidLanguageIndependentTypeName(string value)
	{
		return CSharpHelpers.IsValidTypeNameOrIdentifier(value, isTypeName: true);
	}

	public static void ValidateIdentifiers(CodeObject e)
	{
		new CodeValidator().ValidateIdentifiers(e);
	}
}
