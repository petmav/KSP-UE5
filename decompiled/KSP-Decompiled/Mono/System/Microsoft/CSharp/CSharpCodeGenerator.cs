using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

namespace Microsoft.CSharp;

internal sealed class CSharpCodeGenerator : ICodeCompiler, ICodeGenerator
{
	private static readonly char[] s_periodArray = new char[1] { '.' };

	private ExposedTabStringIndentedTextWriter _output;

	private CodeGeneratorOptions _options;

	private CodeTypeDeclaration _currentClass;

	private CodeTypeMember _currentMember;

	private bool _inNestedBinary;

	private readonly IDictionary<string, string> _provOptions;

	private const int ParameterMultilineThreshold = 15;

	private const int MaxLineLength = 80;

	private const GeneratorSupport LanguageSupport = GeneratorSupport.ArraysOfArrays | GeneratorSupport.EntryPointMethod | GeneratorSupport.GotoStatements | GeneratorSupport.MultidimensionalArrays | GeneratorSupport.StaticConstructors | GeneratorSupport.TryCatchStatements | GeneratorSupport.ReturnTypeAttributes | GeneratorSupport.DeclareValueTypes | GeneratorSupport.DeclareEnums | GeneratorSupport.DeclareDelegates | GeneratorSupport.DeclareInterfaces | GeneratorSupport.DeclareEvents | GeneratorSupport.AssemblyAttributes | GeneratorSupport.ParameterAttributes | GeneratorSupport.ReferenceParameters | GeneratorSupport.ChainedConstructorArguments | GeneratorSupport.NestedTypes | GeneratorSupport.MultipleInterfaceMembers | GeneratorSupport.PublicStaticMembers | GeneratorSupport.ComplexExpressions | GeneratorSupport.Win32Resources | GeneratorSupport.Resources | GeneratorSupport.PartialTypes | GeneratorSupport.GenericTypeReference | GeneratorSupport.GenericTypeDeclaration | GeneratorSupport.DeclareIndexerProperties;

	private static readonly string[][] s_keywords = new string[10][]
	{
		null,
		new string[5] { "as", "do", "if", "in", "is" },
		new string[6] { "for", "int", "new", "out", "ref", "try" },
		new string[15]
		{
			"base", "bool", "byte", "case", "char", "else", "enum", "goto", "lock", "long",
			"null", "this", "true", "uint", "void"
		},
		new string[14]
		{
			"break", "catch", "class", "const", "event", "false", "fixed", "float", "sbyte", "short",
			"throw", "ulong", "using", "while"
		},
		new string[15]
		{
			"double", "extern", "object", "params", "public", "return", "sealed", "sizeof", "static", "string",
			"struct", "switch", "typeof", "unsafe", "ushort"
		},
		new string[7] { "checked", "decimal", "default", "finally", "foreach", "private", "virtual" },
		new string[10] { "abstract", "continue", "delegate", "explicit", "implicit", "internal", "operator", "override", "readonly", "volatile" },
		new string[7] { "__arglist", "__makeref", "__reftype", "interface", "namespace", "protected", "unchecked" },
		new string[2] { "__refvalue", "stackalloc" }
	};

	private bool _generatingForLoop;

	private const string ErrorRegexPattern = "\n\t\t\t^\n\t\t\t(\\s*(?<file>[^\\(]+)                         # filename (optional)\n\t\t\t (\\((?<line>\\d*)(,(?<column>\\d*[\\+]*))?\\))? # line+column (optional)\n\t\t\t :\\s+)?\n\t\t\t(?<level>\\w+)                               # error|warning\n\t\t\t\\s+\n\t\t\t(?<number>[^:]*\\d)                          # CS1234\n\t\t\t:\n\t\t\t\\s*\n\t\t\t(?<message>.*)$";

	private static readonly Regex RelatedSymbolsRegex = new Regex("\n            \\(Location\\ of\\ the\\ symbol\\ related\\ to\\ previous\\ (warning|error)\\)\n\t\t\t", RegexOptions.ExplicitCapture | RegexOptions.Compiled | RegexOptions.IgnorePatternWhitespace);

	private string FileExtension => ".cs";

	private string CompilerName => "csc.exe";

	private string CurrentTypeName
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

	private int Indent
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

	private bool IsCurrentClass
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

	private bool IsCurrentStruct
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

	private string NullToken => "null";

	private CodeGeneratorOptions Options => _options;

	private TextWriter Output => _output;

	internal CSharpCodeGenerator()
	{
	}

	internal CSharpCodeGenerator(IDictionary<string, string> providerOptions)
	{
		_provOptions = providerOptions;
	}

	private string QuoteSnippetStringCStyle(string value)
	{
		StringBuilder stringBuilder = new StringBuilder(value.Length + 5);
		Indentation indentation = new Indentation(_output, Indent + 1);
		stringBuilder.Append('"');
		for (int i = 0; i < value.Length; i++)
		{
			switch (value[i])
			{
			case '\r':
				stringBuilder.Append("\\r");
				break;
			case '\t':
				stringBuilder.Append("\\t");
				break;
			case '"':
				stringBuilder.Append("\\\"");
				break;
			case '\'':
				stringBuilder.Append("\\'");
				break;
			case '\\':
				stringBuilder.Append("\\\\");
				break;
			case '\0':
				stringBuilder.Append("\\0");
				break;
			case '\n':
				stringBuilder.Append("\\n");
				break;
			case '\u2028':
			case '\u2029':
				AppendEscapedChar(stringBuilder, value[i]);
				break;
			default:
				stringBuilder.Append(value[i]);
				break;
			}
			if (i > 0 && i % 80 == 0)
			{
				if (char.IsHighSurrogate(value[i]) && i < value.Length - 1 && char.IsLowSurrogate(value[i + 1]))
				{
					stringBuilder.Append(value[++i]);
				}
				stringBuilder.Append("\" +");
				stringBuilder.Append(Environment.NewLine);
				stringBuilder.Append(indentation.IndentationString);
				stringBuilder.Append('"');
			}
		}
		stringBuilder.Append('"');
		return stringBuilder.ToString();
	}

	private string QuoteSnippetStringVerbatimStyle(string value)
	{
		StringBuilder stringBuilder = new StringBuilder(value.Length + 5);
		stringBuilder.Append("@\"");
		for (int i = 0; i < value.Length; i++)
		{
			if (value[i] == '"')
			{
				stringBuilder.Append("\"\"");
			}
			else
			{
				stringBuilder.Append(value[i]);
			}
		}
		stringBuilder.Append('"');
		return stringBuilder.ToString();
	}

	private string QuoteSnippetString(string value)
	{
		if (value.Length < 256 || value.Length > 1500 || value.IndexOf('\0') != -1)
		{
			return QuoteSnippetStringCStyle(value);
		}
		return QuoteSnippetStringVerbatimStyle(value);
	}

	private void ContinueOnNewLine(string st)
	{
		Output.WriteLine(st);
	}

	private void OutputIdentifier(string ident)
	{
		Output.Write(CreateEscapedIdentifier(ident));
	}

	private void OutputType(CodeTypeReference typeRef)
	{
		Output.Write(GetTypeOutput(typeRef));
	}

	private void GenerateArrayCreateExpression(CodeArrayCreateExpression e)
	{
		Output.Write("new ");
		CodeExpressionCollection initializers = e.Initializers;
		if (initializers.Count > 0)
		{
			OutputType(e.CreateType);
			if (e.CreateType.ArrayRank == 0)
			{
				Output.Write("[]");
			}
			Output.WriteLine(" {");
			Indent++;
			OutputExpressionList(initializers, newlineBetweenItems: true);
			Indent--;
			Output.Write('}');
			return;
		}
		Output.Write(GetBaseTypeOutput(e.CreateType));
		Output.Write('[');
		if (e.SizeExpression != null)
		{
			GenerateExpression(e.SizeExpression);
		}
		else
		{
			Output.Write(e.Size);
		}
		Output.Write(']');
		int nestedArrayDepth = e.CreateType.NestedArrayDepth;
		for (int i = 0; i < nestedArrayDepth - 1; i++)
		{
			Output.Write("[]");
		}
	}

	private void GenerateBaseReferenceExpression(CodeBaseReferenceExpression e)
	{
		Output.Write("base");
	}

	private void GenerateBinaryOperatorExpression(CodeBinaryOperatorExpression e)
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

	private void GenerateCastExpression(CodeCastExpression e)
	{
		Output.Write("((");
		OutputType(e.TargetType);
		Output.Write(")(");
		GenerateExpression(e.Expression);
		Output.Write("))");
	}

	public void GenerateCodeFromMember(CodeTypeMember member, TextWriter writer, CodeGeneratorOptions options)
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

	private void GenerateDefaultValueExpression(CodeDefaultValueExpression e)
	{
		Output.Write("default(");
		OutputType(e.Type);
		Output.Write(')');
	}

	private void GenerateDelegateCreateExpression(CodeDelegateCreateExpression e)
	{
		Output.Write("new ");
		OutputType(e.DelegateType);
		Output.Write('(');
		GenerateExpression(e.TargetObject);
		Output.Write('.');
		OutputIdentifier(e.MethodName);
		Output.Write(')');
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

	private void GenerateFieldReferenceExpression(CodeFieldReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			GenerateExpression(e.TargetObject);
			Output.Write('.');
		}
		OutputIdentifier(e.FieldName);
	}

	private void GenerateArgumentReferenceExpression(CodeArgumentReferenceExpression e)
	{
		OutputIdentifier(e.ParameterName);
	}

	private void GenerateVariableReferenceExpression(CodeVariableReferenceExpression e)
	{
		OutputIdentifier(e.VariableName);
	}

	private void GenerateIndexerExpression(CodeIndexerExpression e)
	{
		GenerateExpression(e.TargetObject);
		Output.Write('[');
		bool flag = true;
		foreach (CodeExpression index in e.Indices)
		{
			if (flag)
			{
				flag = false;
			}
			else
			{
				Output.Write(", ");
			}
			GenerateExpression(index);
		}
		Output.Write(']');
	}

	private void GenerateArrayIndexerExpression(CodeArrayIndexerExpression e)
	{
		GenerateExpression(e.TargetObject);
		Output.Write('[');
		bool flag = true;
		foreach (CodeExpression index in e.Indices)
		{
			if (flag)
			{
				flag = false;
			}
			else
			{
				Output.Write(", ");
			}
			GenerateExpression(index);
		}
		Output.Write(']');
	}

	private void GenerateSnippetCompileUnit(CodeSnippetCompileUnit e)
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

	private void GenerateSnippetExpression(CodeSnippetExpression e)
	{
		Output.Write(e.Value);
	}

	private void GenerateMethodInvokeExpression(CodeMethodInvokeExpression e)
	{
		GenerateMethodReferenceExpression(e.Method);
		Output.Write('(');
		OutputExpressionList(e.Parameters);
		Output.Write(')');
	}

	private void GenerateMethodReferenceExpression(CodeMethodReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			if (e.TargetObject is CodeBinaryOperatorExpression)
			{
				Output.Write('(');
				GenerateExpression(e.TargetObject);
				Output.Write(')');
			}
			else
			{
				GenerateExpression(e.TargetObject);
			}
			Output.Write('.');
		}
		OutputIdentifier(e.MethodName);
		if (e.TypeArguments.Count > 0)
		{
			Output.Write(GetTypeArgumentsOutput(e.TypeArguments));
		}
	}

	private bool GetUserData(CodeObject e, string property, bool defaultValue)
	{
		object obj = e.UserData[property];
		if (obj != null && obj is bool)
		{
			return (bool)obj;
		}
		return defaultValue;
	}

	private void GenerateNamespace(CodeNamespace e)
	{
		GenerateCommentStatements(e.Comments);
		GenerateNamespaceStart(e);
		if (GetUserData(e, "GenerateImports", defaultValue: true))
		{
			GenerateNamespaceImports(e);
		}
		Output.WriteLine();
		GenerateTypes(e);
		GenerateNamespaceEnd(e);
	}

	private void GenerateStatement(CodeStatement e)
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

	private void GenerateStatements(CodeStatementCollection stmts)
	{
		foreach (CodeStatement stmt in stmts)
		{
			((ICodeGenerator)this).GenerateCodeFromStatement(stmt, _output.InnerWriter, _options);
		}
	}

	private void GenerateNamespaceImports(CodeNamespace e)
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

	private void GenerateEventReferenceExpression(CodeEventReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			GenerateExpression(e.TargetObject);
			Output.Write('.');
		}
		OutputIdentifier(e.EventName);
	}

	private void GenerateDelegateInvokeExpression(CodeDelegateInvokeExpression e)
	{
		if (e.TargetObject != null)
		{
			GenerateExpression(e.TargetObject);
		}
		Output.Write('(');
		OutputExpressionList(e.Parameters);
		Output.Write(')');
	}

	private void GenerateObjectCreateExpression(CodeObjectCreateExpression e)
	{
		Output.Write("new ");
		OutputType(e.CreateType);
		Output.Write('(');
		OutputExpressionList(e.Parameters);
		Output.Write(')');
	}

	private void GeneratePrimitiveExpression(CodePrimitiveExpression e)
	{
		if (e.Value is char)
		{
			GeneratePrimitiveChar((char)e.Value);
		}
		else if (e.Value is sbyte)
		{
			Output.Write(((sbyte)e.Value).ToString(CultureInfo.InvariantCulture));
		}
		else if (e.Value is ushort)
		{
			Output.Write(((ushort)e.Value).ToString(CultureInfo.InvariantCulture));
		}
		else if (e.Value is uint)
		{
			Output.Write(((uint)e.Value).ToString(CultureInfo.InvariantCulture));
			Output.Write('u');
		}
		else if (e.Value is ulong)
		{
			Output.Write(((ulong)e.Value).ToString(CultureInfo.InvariantCulture));
			Output.Write("ul");
		}
		else
		{
			GeneratePrimitiveExpressionBase(e);
		}
	}

	private void GeneratePrimitiveExpressionBase(CodePrimitiveExpression e)
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

	private void GeneratePrimitiveChar(char c)
	{
		Output.Write('\'');
		switch (c)
		{
		case '\r':
			Output.Write("\\r");
			break;
		case '\t':
			Output.Write("\\t");
			break;
		case '"':
			Output.Write("\\\"");
			break;
		case '\'':
			Output.Write("\\'");
			break;
		case '\\':
			Output.Write("\\\\");
			break;
		case '\0':
			Output.Write("\\0");
			break;
		case '\n':
			Output.Write("\\n");
			break;
		case '\u0084':
		case '\u0085':
		case '\u2028':
		case '\u2029':
			AppendEscapedChar(null, c);
			break;
		default:
			if (char.IsSurrogate(c))
			{
				AppendEscapedChar(null, c);
			}
			else
			{
				Output.Write(c);
			}
			break;
		}
		Output.Write('\'');
	}

	private void AppendEscapedChar(StringBuilder b, char value)
	{
		if (b == null)
		{
			Output.Write("\\u");
			TextWriter output = Output;
			int num = value;
			output.Write(num.ToString("X4", CultureInfo.InvariantCulture));
		}
		else
		{
			b.Append("\\u");
			int num = value;
			b.Append(num.ToString("X4", CultureInfo.InvariantCulture));
		}
	}

	private void GeneratePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e)
	{
		Output.Write("value");
	}

	private void GenerateThisReferenceExpression(CodeThisReferenceExpression e)
	{
		Output.Write("this");
	}

	private void GenerateExpressionStatement(CodeExpressionStatement e)
	{
		GenerateExpression(e.Expression);
		if (!_generatingForLoop)
		{
			Output.WriteLine(';');
		}
	}

	private void GenerateIterationStatement(CodeIterationStatement e)
	{
		_generatingForLoop = true;
		Output.Write("for (");
		GenerateStatement(e.InitStatement);
		Output.Write("; ");
		GenerateExpression(e.TestExpression);
		Output.Write("; ");
		GenerateStatement(e.IncrementStatement);
		Output.Write(')');
		OutputStartingBrace();
		_generatingForLoop = false;
		Indent++;
		GenerateStatements(e.Statements);
		Indent--;
		Output.WriteLine('}');
	}

	private void GenerateThrowExceptionStatement(CodeThrowExceptionStatement e)
	{
		Output.Write("throw");
		if (e.ToThrow != null)
		{
			Output.Write(' ');
			GenerateExpression(e.ToThrow);
		}
		Output.WriteLine(';');
	}

	private void GenerateComment(CodeComment e)
	{
		string value = (e.DocComment ? "///" : "//");
		Output.Write(value);
		Output.Write(' ');
		string text = e.Text;
		for (int i = 0; i < text.Length; i++)
		{
			if (text[i] == '\0')
			{
				continue;
			}
			Output.Write(text[i]);
			if (text[i] == '\r')
			{
				if (i < text.Length - 1 && text[i + 1] == '\n')
				{
					Output.Write('\n');
					i++;
				}
				_output.InternalOutputTabs();
				Output.Write(value);
			}
			else if (text[i] == '\n')
			{
				_output.InternalOutputTabs();
				Output.Write(value);
			}
			else if (text[i] == '\u2028' || text[i] == '\u2029' || text[i] == '\u0085')
			{
				Output.Write(value);
			}
		}
		Output.WriteLine();
	}

	private void GenerateCommentStatement(CodeCommentStatement e)
	{
		if (e.Comment == null)
		{
			throw new ArgumentException(global::SR.Format("The 'Comment' property of the CodeCommentStatement '{0}' cannot be null.", "e"), "e");
		}
		GenerateComment(e.Comment);
	}

	private void GenerateCommentStatements(CodeCommentStatementCollection e)
	{
		foreach (CodeCommentStatement item in e)
		{
			GenerateCommentStatement(item);
		}
	}

	private void GenerateMethodReturnStatement(CodeMethodReturnStatement e)
	{
		Output.Write("return");
		if (e.Expression != null)
		{
			Output.Write(' ');
			GenerateExpression(e.Expression);
		}
		Output.WriteLine(';');
	}

	private void GenerateConditionStatement(CodeConditionStatement e)
	{
		Output.Write("if (");
		GenerateExpression(e.Condition);
		Output.Write(')');
		OutputStartingBrace();
		Indent++;
		GenerateStatements(e.TrueStatements);
		Indent--;
		if (e.FalseStatements.Count > 0)
		{
			Output.Write('}');
			if (Options.ElseOnClosing)
			{
				Output.Write(' ');
			}
			else
			{
				Output.WriteLine();
			}
			Output.Write("else");
			OutputStartingBrace();
			Indent++;
			GenerateStatements(e.FalseStatements);
			Indent--;
		}
		Output.WriteLine('}');
	}

	private void GenerateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e)
	{
		Output.Write("try");
		OutputStartingBrace();
		Indent++;
		GenerateStatements(e.TryStatements);
		Indent--;
		CodeCatchClauseCollection catchClauses = e.CatchClauses;
		if (catchClauses.Count > 0)
		{
			foreach (CodeCatchClause item in catchClauses)
			{
				Output.Write('}');
				if (Options.ElseOnClosing)
				{
					Output.Write(' ');
				}
				else
				{
					Output.WriteLine();
				}
				Output.Write("catch (");
				OutputType(item.CatchExceptionType);
				Output.Write(' ');
				OutputIdentifier(item.LocalName);
				Output.Write(')');
				OutputStartingBrace();
				Indent++;
				GenerateStatements(item.Statements);
				Indent--;
			}
		}
		CodeStatementCollection finallyStatements = e.FinallyStatements;
		if (finallyStatements.Count > 0)
		{
			Output.Write('}');
			if (Options.ElseOnClosing)
			{
				Output.Write(' ');
			}
			else
			{
				Output.WriteLine();
			}
			Output.Write("finally");
			OutputStartingBrace();
			Indent++;
			GenerateStatements(finallyStatements);
			Indent--;
		}
		Output.WriteLine('}');
	}

	private void GenerateAssignStatement(CodeAssignStatement e)
	{
		GenerateExpression(e.Left);
		Output.Write(" = ");
		GenerateExpression(e.Right);
		if (!_generatingForLoop)
		{
			Output.WriteLine(';');
		}
	}

	private void GenerateAttachEventStatement(CodeAttachEventStatement e)
	{
		GenerateEventReferenceExpression(e.Event);
		Output.Write(" += ");
		GenerateExpression(e.Listener);
		Output.WriteLine(';');
	}

	private void GenerateRemoveEventStatement(CodeRemoveEventStatement e)
	{
		GenerateEventReferenceExpression(e.Event);
		Output.Write(" -= ");
		GenerateExpression(e.Listener);
		Output.WriteLine(';');
	}

	private void GenerateSnippetStatement(CodeSnippetStatement e)
	{
		Output.WriteLine(e.Value);
	}

	private void GenerateGotoStatement(CodeGotoStatement e)
	{
		Output.Write("goto ");
		Output.Write(e.Label);
		Output.WriteLine(';');
	}

	private void GenerateLabeledStatement(CodeLabeledStatement e)
	{
		Indent--;
		Output.Write(e.Label);
		Output.WriteLine(':');
		Indent++;
		if (e.Statement != null)
		{
			GenerateStatement(e.Statement);
		}
	}

	private void GenerateVariableDeclarationStatement(CodeVariableDeclarationStatement e)
	{
		OutputTypeNamePair(e.Type, e.Name);
		if (e.InitExpression != null)
		{
			Output.Write(" = ");
			GenerateExpression(e.InitExpression);
		}
		if (!_generatingForLoop)
		{
			Output.WriteLine(';');
		}
	}

	private void GenerateLinePragmaStart(CodeLinePragma e)
	{
		Output.WriteLine();
		Output.Write("#line ");
		Output.Write(e.LineNumber);
		Output.Write(" \"");
		Output.Write(e.FileName);
		Output.Write('"');
		Output.WriteLine();
	}

	private void GenerateLinePragmaEnd(CodeLinePragma e)
	{
		Output.WriteLine();
		Output.WriteLine("#line default");
		Output.WriteLine("#line hidden");
	}

	private void GenerateEvent(CodeMemberEvent e, CodeTypeDeclaration c)
	{
		if (!IsCurrentDelegate && !IsCurrentEnum)
		{
			if (e.CustomAttributes.Count > 0)
			{
				GenerateAttributes(e.CustomAttributes);
			}
			if (e.PrivateImplementationType == null)
			{
				OutputMemberAccessModifier(e.Attributes);
			}
			Output.Write("event ");
			string text = e.Name;
			if (e.PrivateImplementationType != null)
			{
				text = GetBaseTypeOutput(e.PrivateImplementationType, preferBuiltInTypes: false) + "." + text;
			}
			OutputTypeNamePair(e.Type, text);
			Output.WriteLine(';');
		}
	}

	private void GenerateExpression(CodeExpression e)
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

	private void GenerateField(CodeMemberField e)
	{
		if (IsCurrentDelegate || IsCurrentInterface)
		{
			return;
		}
		if (IsCurrentEnum)
		{
			if (e.CustomAttributes.Count > 0)
			{
				GenerateAttributes(e.CustomAttributes);
			}
			OutputIdentifier(e.Name);
			if (e.InitExpression != null)
			{
				Output.Write(" = ");
				GenerateExpression(e.InitExpression);
			}
			Output.WriteLine(',');
			return;
		}
		if (e.CustomAttributes.Count > 0)
		{
			GenerateAttributes(e.CustomAttributes);
		}
		OutputMemberAccessModifier(e.Attributes);
		OutputVTableModifier(e.Attributes);
		OutputFieldScopeModifier(e.Attributes);
		OutputTypeNamePair(e.Type, e.Name);
		if (e.InitExpression != null)
		{
			Output.Write(" = ");
			GenerateExpression(e.InitExpression);
		}
		Output.WriteLine(';');
	}

	private void GenerateSnippetMember(CodeSnippetTypeMember e)
	{
		Output.Write(e.Text);
	}

	private void GenerateParameterDeclarationExpression(CodeParameterDeclarationExpression e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			GenerateAttributes(e.CustomAttributes, null, inLine: true);
		}
		OutputDirection(e.Direction);
		OutputTypeNamePair(e.Type, e.Name);
	}

	private void GenerateEntryPointMethod(CodeEntryPointMethod e, CodeTypeDeclaration c)
	{
		if (e.CustomAttributes.Count > 0)
		{
			GenerateAttributes(e.CustomAttributes);
		}
		Output.Write("public static ");
		OutputType(e.ReturnType);
		Output.Write(" Main()");
		OutputStartingBrace();
		Indent++;
		GenerateStatements(e.Statements);
		Indent--;
		Output.WriteLine('}');
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

	private void GenerateMethod(CodeMemberMethod e, CodeTypeDeclaration c)
	{
		if (!IsCurrentClass && !IsCurrentStruct && !IsCurrentInterface)
		{
			return;
		}
		if (e.CustomAttributes.Count > 0)
		{
			GenerateAttributes(e.CustomAttributes);
		}
		if (e.ReturnTypeCustomAttributes.Count > 0)
		{
			GenerateAttributes(e.ReturnTypeCustomAttributes, "return: ");
		}
		if (!IsCurrentInterface)
		{
			if (e.PrivateImplementationType == null)
			{
				OutputMemberAccessModifier(e.Attributes);
				OutputVTableModifier(e.Attributes);
				OutputMemberScopeModifier(e.Attributes);
			}
		}
		else
		{
			OutputVTableModifier(e.Attributes);
		}
		OutputType(e.ReturnType);
		Output.Write(' ');
		if (e.PrivateImplementationType != null)
		{
			Output.Write(GetBaseTypeOutput(e.PrivateImplementationType, preferBuiltInTypes: false));
			Output.Write('.');
		}
		OutputIdentifier(e.Name);
		OutputTypeParameters(e.TypeParameters);
		Output.Write('(');
		OutputParameters(e.Parameters);
		Output.Write(')');
		OutputTypeParameterConstraints(e.TypeParameters);
		if (!IsCurrentInterface && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract)
		{
			OutputStartingBrace();
			Indent++;
			GenerateStatements(e.Statements);
			Indent--;
			Output.WriteLine('}');
		}
		else
		{
			Output.WriteLine(';');
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

	private void GenerateProperty(CodeMemberProperty e, CodeTypeDeclaration c)
	{
		if (!IsCurrentClass && !IsCurrentStruct && !IsCurrentInterface)
		{
			return;
		}
		if (e.CustomAttributes.Count > 0)
		{
			GenerateAttributes(e.CustomAttributes);
		}
		if (!IsCurrentInterface)
		{
			if (e.PrivateImplementationType == null)
			{
				OutputMemberAccessModifier(e.Attributes);
				OutputVTableModifier(e.Attributes);
				OutputMemberScopeModifier(e.Attributes);
			}
		}
		else
		{
			OutputVTableModifier(e.Attributes);
		}
		OutputType(e.Type);
		Output.Write(' ');
		if (e.PrivateImplementationType != null && !IsCurrentInterface)
		{
			Output.Write(GetBaseTypeOutput(e.PrivateImplementationType, preferBuiltInTypes: false));
			Output.Write('.');
		}
		if (e.Parameters.Count > 0 && string.Equals(e.Name, "Item", StringComparison.OrdinalIgnoreCase))
		{
			Output.Write("this[");
			OutputParameters(e.Parameters);
			Output.Write(']');
		}
		else
		{
			OutputIdentifier(e.Name);
		}
		OutputStartingBrace();
		Indent++;
		if (e.HasGet)
		{
			if (IsCurrentInterface || (e.Attributes & MemberAttributes.ScopeMask) == MemberAttributes.Abstract)
			{
				Output.WriteLine("get;");
			}
			else
			{
				Output.Write("get");
				OutputStartingBrace();
				Indent++;
				GenerateStatements(e.GetStatements);
				Indent--;
				Output.WriteLine('}');
			}
		}
		if (e.HasSet)
		{
			if (IsCurrentInterface || (e.Attributes & MemberAttributes.ScopeMask) == MemberAttributes.Abstract)
			{
				Output.WriteLine("set;");
			}
			else
			{
				Output.Write("set");
				OutputStartingBrace();
				Indent++;
				GenerateStatements(e.SetStatements);
				Indent--;
				Output.WriteLine('}');
			}
		}
		Indent--;
		Output.WriteLine('}');
	}

	private void GenerateSingleFloatValue(float s)
	{
		if (float.IsNaN(s))
		{
			Output.Write("float.NaN");
			return;
		}
		if (float.IsNegativeInfinity(s))
		{
			Output.Write("float.NegativeInfinity");
			return;
		}
		if (float.IsPositiveInfinity(s))
		{
			Output.Write("float.PositiveInfinity");
			return;
		}
		Output.Write(s.ToString(CultureInfo.InvariantCulture));
		Output.Write('F');
	}

	private void GenerateDoubleValue(double d)
	{
		if (double.IsNaN(d))
		{
			Output.Write("double.NaN");
			return;
		}
		if (double.IsNegativeInfinity(d))
		{
			Output.Write("double.NegativeInfinity");
			return;
		}
		if (double.IsPositiveInfinity(d))
		{
			Output.Write("double.PositiveInfinity");
			return;
		}
		Output.Write(d.ToString("R", CultureInfo.InvariantCulture));
		Output.Write('D');
	}

	private void GenerateDecimalValue(decimal d)
	{
		Output.Write(d.ToString(CultureInfo.InvariantCulture));
		Output.Write('m');
	}

	private void OutputVTableModifier(MemberAttributes attributes)
	{
		MemberAttributes memberAttributes = attributes & MemberAttributes.VTableMask;
		if (memberAttributes == MemberAttributes.New)
		{
			Output.Write("new ");
		}
	}

	private void OutputMemberAccessModifier(MemberAttributes attributes)
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

	private void OutputMemberScopeModifier(MemberAttributes attributes)
	{
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
		MemberAttributes memberAttributes = attributes & MemberAttributes.AccessMask;
		if (memberAttributes == MemberAttributes.Assembly || memberAttributes == MemberAttributes.Family || memberAttributes == MemberAttributes.Public)
		{
			Output.Write("virtual ");
		}
	}

	private void OutputOperator(CodeBinaryOperatorType op)
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

	private void OutputFieldScopeModifier(MemberAttributes attributes)
	{
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

	private void GeneratePropertyReferenceExpression(CodePropertyReferenceExpression e)
	{
		if (e.TargetObject != null)
		{
			GenerateExpression(e.TargetObject);
			Output.Write('.');
		}
		OutputIdentifier(e.PropertyName);
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

	private void GenerateConstructor(CodeConstructor e, CodeTypeDeclaration c)
	{
		if (IsCurrentClass || IsCurrentStruct)
		{
			if (e.CustomAttributes.Count > 0)
			{
				GenerateAttributes(e.CustomAttributes);
			}
			OutputMemberAccessModifier(e.Attributes);
			OutputIdentifier(CurrentTypeName);
			Output.Write('(');
			OutputParameters(e.Parameters);
			Output.Write(')');
			CodeExpressionCollection baseConstructorArgs = e.BaseConstructorArgs;
			CodeExpressionCollection chainedConstructorArgs = e.ChainedConstructorArgs;
			if (baseConstructorArgs.Count > 0)
			{
				Output.WriteLine(" : ");
				Indent++;
				Indent++;
				Output.Write("base(");
				OutputExpressionList(baseConstructorArgs);
				Output.Write(')');
				Indent--;
				Indent--;
			}
			if (chainedConstructorArgs.Count > 0)
			{
				Output.WriteLine(" : ");
				Indent++;
				Indent++;
				Output.Write("this(");
				OutputExpressionList(chainedConstructorArgs);
				Output.Write(')');
				Indent--;
				Indent--;
			}
			OutputStartingBrace();
			Indent++;
			GenerateStatements(e.Statements);
			Indent--;
			Output.WriteLine('}');
		}
	}

	private void GenerateTypeConstructor(CodeTypeConstructor e)
	{
		if (IsCurrentClass || IsCurrentStruct)
		{
			if (e.CustomAttributes.Count > 0)
			{
				GenerateAttributes(e.CustomAttributes);
			}
			Output.Write("static ");
			Output.Write(CurrentTypeName);
			Output.Write("()");
			OutputStartingBrace();
			Indent++;
			GenerateStatements(e.Statements);
			Indent--;
			Output.WriteLine('}');
		}
	}

	private void GenerateTypeReferenceExpression(CodeTypeReferenceExpression e)
	{
		OutputType(e.Type);
	}

	private void GenerateTypeOfExpression(CodeTypeOfExpression e)
	{
		Output.Write("typeof(");
		OutputType(e.Type);
		Output.Write(')');
	}

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

	private void GenerateTypes(CodeNamespace e)
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

	private void GenerateTypeStart(CodeTypeDeclaration e)
	{
		if (e.CustomAttributes.Count > 0)
		{
			GenerateAttributes(e.CustomAttributes);
		}
		if (IsCurrentDelegate)
		{
			TypeAttributes typeAttributes = e.TypeAttributes & TypeAttributes.VisibilityMask;
			if (typeAttributes != 0 && typeAttributes == TypeAttributes.Public)
			{
				Output.Write("public ");
			}
			CodeTypeDelegate codeTypeDelegate = (CodeTypeDelegate)e;
			Output.Write("delegate ");
			OutputType(codeTypeDelegate.ReturnType);
			Output.Write(' ');
			OutputIdentifier(e.Name);
			Output.Write('(');
			OutputParameters(codeTypeDelegate.Parameters);
			Output.WriteLine(");");
			return;
		}
		OutputTypeAttributes(e);
		OutputIdentifier(e.Name);
		OutputTypeParameters(e.TypeParameters);
		bool flag = true;
		foreach (CodeTypeReference baseType in e.BaseTypes)
		{
			if (flag)
			{
				Output.Write(" : ");
				flag = false;
			}
			else
			{
				Output.Write(", ");
			}
			OutputType(baseType);
		}
		OutputTypeParameterConstraints(e.TypeParameters);
		OutputStartingBrace();
		Indent++;
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

	private void GenerateNamespaces(CodeCompileUnit e)
	{
		foreach (CodeNamespace @namespace in e.Namespaces)
		{
			((ICodeGenerator)this).GenerateCodeFromNamespace(@namespace, _output.InnerWriter, _options);
		}
	}

	private void OutputAttributeArgument(CodeAttributeArgument arg)
	{
		if (!string.IsNullOrEmpty(arg.Name))
		{
			OutputIdentifier(arg.Name);
			Output.Write('=');
		}
		((ICodeGenerator)this).GenerateCodeFromExpression(arg.Value, _output.InnerWriter, _options);
	}

	private void OutputDirection(FieldDirection dir)
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

	private void OutputExpressionList(CodeExpressionCollection expressions)
	{
		OutputExpressionList(expressions, newlineBetweenItems: false);
	}

	private void OutputExpressionList(CodeExpressionCollection expressions, bool newlineBetweenItems)
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

	private void OutputParameters(CodeParameterDeclarationExpressionCollection parameters)
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

	private void OutputTypeNamePair(CodeTypeReference typeRef, string name)
	{
		OutputType(typeRef);
		Output.Write(' ');
		OutputIdentifier(name);
	}

	private void OutputTypeParameters(CodeTypeParameterCollection typeParameters)
	{
		if (typeParameters.Count == 0)
		{
			return;
		}
		Output.Write('<');
		bool flag = true;
		for (int i = 0; i < typeParameters.Count; i++)
		{
			if (flag)
			{
				flag = false;
			}
			else
			{
				Output.Write(", ");
			}
			if (typeParameters[i].CustomAttributes.Count > 0)
			{
				GenerateAttributes(typeParameters[i].CustomAttributes, null, inLine: true);
				Output.Write(' ');
			}
			Output.Write(typeParameters[i].Name);
		}
		Output.Write('>');
	}

	private void OutputTypeParameterConstraints(CodeTypeParameterCollection typeParameters)
	{
		if (typeParameters.Count == 0)
		{
			return;
		}
		for (int i = 0; i < typeParameters.Count; i++)
		{
			Output.WriteLine();
			Indent++;
			bool flag = true;
			if (typeParameters[i].Constraints.Count > 0)
			{
				foreach (CodeTypeReference constraint in typeParameters[i].Constraints)
				{
					if (flag)
					{
						Output.Write("where ");
						Output.Write(typeParameters[i].Name);
						Output.Write(" : ");
						flag = false;
					}
					else
					{
						Output.Write(", ");
					}
					OutputType(constraint);
				}
			}
			if (typeParameters[i].HasConstructorConstraint)
			{
				if (flag)
				{
					Output.Write("where ");
					Output.Write(typeParameters[i].Name);
					Output.Write(" : new()");
				}
				else
				{
					Output.Write(", new ()");
				}
			}
			Indent--;
		}
	}

	private void OutputTypeAttributes(CodeTypeDeclaration e)
	{
		if ((e.Attributes & MemberAttributes.New) != 0)
		{
			Output.Write("new ");
		}
		TypeAttributes typeAttributes = e.TypeAttributes;
		switch (typeAttributes & TypeAttributes.VisibilityMask)
		{
		case TypeAttributes.Public:
		case TypeAttributes.NestedPublic:
			Output.Write("public ");
			break;
		case TypeAttributes.NestedPrivate:
			Output.Write("private ");
			break;
		case TypeAttributes.NestedFamily:
			Output.Write("protected ");
			break;
		case TypeAttributes.NotPublic:
		case TypeAttributes.NestedAssembly:
		case TypeAttributes.NestedFamANDAssem:
			Output.Write("internal ");
			break;
		case TypeAttributes.VisibilityMask:
			Output.Write("protected internal ");
			break;
		}
		if (e.IsStruct)
		{
			if (e.IsPartial)
			{
				Output.Write("partial ");
			}
			Output.Write("struct ");
			return;
		}
		if (e.IsEnum)
		{
			Output.Write("enum ");
			return;
		}
		switch (typeAttributes & TypeAttributes.ClassSemanticsMask)
		{
		case TypeAttributes.NotPublic:
			if ((typeAttributes & TypeAttributes.Sealed) == TypeAttributes.Sealed)
			{
				Output.Write("sealed ");
			}
			if ((typeAttributes & TypeAttributes.Abstract) == TypeAttributes.Abstract)
			{
				Output.Write("abstract ");
			}
			if (e.IsPartial)
			{
				Output.Write("partial ");
			}
			Output.Write("class ");
			break;
		case TypeAttributes.ClassSemanticsMask:
			if (e.IsPartial)
			{
				Output.Write("partial ");
			}
			Output.Write("interface ");
			break;
		}
	}

	private void GenerateTypeEnd(CodeTypeDeclaration e)
	{
		if (!IsCurrentDelegate)
		{
			Indent--;
			Output.WriteLine('}');
		}
	}

	private void GenerateNamespaceStart(CodeNamespace e)
	{
		if (!string.IsNullOrEmpty(e.Name))
		{
			Output.Write("namespace ");
			string[] array = e.Name.Split(s_periodArray);
			OutputIdentifier(array[0]);
			for (int i = 1; i < array.Length; i++)
			{
				Output.Write('.');
				OutputIdentifier(array[i]);
			}
			OutputStartingBrace();
			Indent++;
		}
	}

	private void GenerateCompileUnit(CodeCompileUnit e)
	{
		GenerateCompileUnitStart(e);
		GenerateNamespaces(e);
		GenerateCompileUnitEnd(e);
	}

	private void GenerateCompileUnitStart(CodeCompileUnit e)
	{
		if (e.StartDirectives.Count > 0)
		{
			GenerateDirectives(e.StartDirectives);
		}
		Output.WriteLine("//------------------------------------------------------------------------------");
		Output.Write("// <");
		Output.WriteLine("auto-generated>");
		Output.Write("//     ");
		Output.WriteLine("This code was generated by a tool.");
		Output.Write("//     ");
		Output.Write("Runtime Version:");
		Output.WriteLine(Environment.Version.ToString());
		Output.WriteLine("//");
		Output.Write("//     ");
		Output.WriteLine("Changes to this file may cause incorrect behavior and will be lost if");
		Output.Write("//     ");
		Output.WriteLine("the code is regenerated.");
		Output.Write("// </");
		Output.WriteLine("auto-generated>");
		Output.WriteLine("//------------------------------------------------------------------------------");
		Output.WriteLine();
		SortedSet<string> sortedSet = new SortedSet<string>(StringComparer.Ordinal);
		foreach (CodeNamespace @namespace in e.Namespaces)
		{
			if (!string.IsNullOrEmpty(@namespace.Name))
			{
				continue;
			}
			@namespace.UserData["GenerateImports"] = false;
			foreach (CodeNamespaceImport import in @namespace.Imports)
			{
				sortedSet.Add(import.Namespace);
			}
		}
		foreach (string item in sortedSet)
		{
			Output.Write("using ");
			OutputIdentifier(item);
			Output.WriteLine(';');
		}
		if (sortedSet.Count > 0)
		{
			Output.WriteLine();
		}
		if (e.AssemblyCustomAttributes.Count > 0)
		{
			GenerateAttributes(e.AssemblyCustomAttributes, "assembly: ");
			Output.WriteLine();
		}
	}

	private void GenerateCompileUnitEnd(CodeCompileUnit e)
	{
		if (e.EndDirectives.Count > 0)
		{
			GenerateDirectives(e.EndDirectives);
		}
	}

	private void GenerateDirectionExpression(CodeDirectionExpression e)
	{
		OutputDirection(e.Direction);
		GenerateExpression(e.Expression);
	}

	private void GenerateDirectives(CodeDirectiveCollection directives)
	{
		for (int i = 0; i < directives.Count; i++)
		{
			CodeDirective codeDirective = directives[i];
			if (codeDirective is CodeChecksumPragma)
			{
				GenerateChecksumPragma((CodeChecksumPragma)codeDirective);
			}
			else if (codeDirective is CodeRegionDirective)
			{
				GenerateCodeRegionDirective((CodeRegionDirective)codeDirective);
			}
		}
	}

	private void GenerateChecksumPragma(CodeChecksumPragma checksumPragma)
	{
		Output.Write("#pragma checksum \"");
		Output.Write(checksumPragma.FileName);
		Output.Write("\" \"");
		Output.Write(checksumPragma.ChecksumAlgorithmId.ToString("B", CultureInfo.InvariantCulture));
		Output.Write("\" \"");
		if (checksumPragma.ChecksumData != null)
		{
			byte[] checksumData = checksumPragma.ChecksumData;
			foreach (byte b in checksumData)
			{
				Output.Write(b.ToString("X2", CultureInfo.InvariantCulture));
			}
		}
		Output.WriteLine("\"");
	}

	private void GenerateCodeRegionDirective(CodeRegionDirective regionDirective)
	{
		if (regionDirective.RegionMode == CodeRegionMode.Start)
		{
			Output.Write("#region ");
			Output.WriteLine(regionDirective.RegionText);
		}
		else if (regionDirective.RegionMode == CodeRegionMode.End)
		{
			Output.WriteLine("#endregion");
		}
	}

	private void GenerateNamespaceEnd(CodeNamespace e)
	{
		if (!string.IsNullOrEmpty(e.Name))
		{
			Indent--;
			Output.WriteLine('}');
		}
	}

	private void GenerateNamespaceImport(CodeNamespaceImport e)
	{
		Output.Write("using ");
		OutputIdentifier(e.Namespace);
		Output.WriteLine(';');
	}

	private void GenerateAttributeDeclarationsStart(CodeAttributeDeclarationCollection attributes)
	{
		Output.Write('[');
	}

	private void GenerateAttributeDeclarationsEnd(CodeAttributeDeclarationCollection attributes)
	{
		Output.Write(']');
	}

	private void GenerateAttributes(CodeAttributeDeclarationCollection attributes)
	{
		GenerateAttributes(attributes, null, inLine: false);
	}

	private void GenerateAttributes(CodeAttributeDeclarationCollection attributes, string prefix)
	{
		GenerateAttributes(attributes, prefix, inLine: false);
	}

	private void GenerateAttributes(CodeAttributeDeclarationCollection attributes, string prefix, bool inLine)
	{
		if (attributes.Count == 0)
		{
			return;
		}
		bool flag = false;
		foreach (CodeAttributeDeclaration attribute in attributes)
		{
			if (attribute.Name.Equals("system.paramarrayattribute", StringComparison.OrdinalIgnoreCase))
			{
				flag = true;
				continue;
			}
			GenerateAttributeDeclarationsStart(attributes);
			if (prefix != null)
			{
				Output.Write(prefix);
			}
			if (attribute.AttributeType != null)
			{
				Output.Write(GetTypeOutput(attribute.AttributeType));
			}
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
			GenerateAttributeDeclarationsEnd(attributes);
			if (inLine)
			{
				Output.Write(' ');
			}
			else
			{
				Output.WriteLine();
			}
		}
		if (flag)
		{
			if (prefix != null)
			{
				Output.Write(prefix);
			}
			Output.Write("params");
			if (inLine)
			{
				Output.Write(' ');
			}
			else
			{
				Output.WriteLine();
			}
		}
	}

	public bool Supports(GeneratorSupport support)
	{
		return (support & (GeneratorSupport.ArraysOfArrays | GeneratorSupport.EntryPointMethod | GeneratorSupport.GotoStatements | GeneratorSupport.MultidimensionalArrays | GeneratorSupport.StaticConstructors | GeneratorSupport.TryCatchStatements | GeneratorSupport.ReturnTypeAttributes | GeneratorSupport.DeclareValueTypes | GeneratorSupport.DeclareEnums | GeneratorSupport.DeclareDelegates | GeneratorSupport.DeclareInterfaces | GeneratorSupport.DeclareEvents | GeneratorSupport.AssemblyAttributes | GeneratorSupport.ParameterAttributes | GeneratorSupport.ReferenceParameters | GeneratorSupport.ChainedConstructorArguments | GeneratorSupport.NestedTypes | GeneratorSupport.MultipleInterfaceMembers | GeneratorSupport.PublicStaticMembers | GeneratorSupport.ComplexExpressions | GeneratorSupport.Win32Resources | GeneratorSupport.Resources | GeneratorSupport.PartialTypes | GeneratorSupport.GenericTypeReference | GeneratorSupport.GenericTypeDeclaration | GeneratorSupport.DeclareIndexerProperties)) == support;
	}

	public bool IsValidIdentifier(string value)
	{
		if (string.IsNullOrEmpty(value))
		{
			return false;
		}
		if (value.Length > 512)
		{
			return false;
		}
		if (value[0] != '@')
		{
			if (CSharpHelpers.IsKeyword(value))
			{
				return false;
			}
		}
		else
		{
			value = value.Substring(1);
		}
		return CodeGenerator.IsValidLanguageIndependentIdentifier(value);
	}

	public void ValidateIdentifier(string value)
	{
		if (!IsValidIdentifier(value))
		{
			throw new ArgumentException(global::SR.Format("Identifier '{0}' is not valid.", value));
		}
	}

	public string CreateValidIdentifier(string name)
	{
		if (CSharpHelpers.IsPrefixTwoUnderscore(name))
		{
			name = "_" + name;
		}
		while (CSharpHelpers.IsKeyword(name))
		{
			name = "_" + name;
		}
		return name;
	}

	public string CreateEscapedIdentifier(string name)
	{
		return CSharpHelpers.CreateEscapedIdentifier(name);
	}

	private string GetBaseTypeOutput(CodeTypeReference typeRef, bool preferBuiltInTypes = true)
	{
		string baseType = typeRef.BaseType;
		if (preferBuiltInTypes)
		{
			if (baseType.Length == 0)
			{
				return "void";
			}
			switch (baseType.ToLower(CultureInfo.InvariantCulture).Trim())
			{
			case "system.int16":
				return "short";
			case "system.int32":
				return "int";
			case "system.int64":
				return "long";
			case "system.string":
				return "string";
			case "system.object":
				return "object";
			case "system.boolean":
				return "bool";
			case "system.void":
				return "void";
			case "system.char":
				return "char";
			case "system.byte":
				return "byte";
			case "system.uint16":
				return "ushort";
			case "system.uint32":
				return "uint";
			case "system.uint64":
				return "ulong";
			case "system.sbyte":
				return "sbyte";
			case "system.single":
				return "float";
			case "system.double":
				return "double";
			case "system.decimal":
				return "decimal";
			}
		}
		StringBuilder stringBuilder = new StringBuilder(baseType.Length + 10);
		if ((typeRef.Options & CodeTypeReferenceOptions.GlobalReference) != 0)
		{
			stringBuilder.Append("global::");
		}
		string baseType2 = typeRef.BaseType;
		int num = 0;
		int num2 = 0;
		for (int i = 0; i < baseType2.Length; i++)
		{
			switch (baseType2[i])
			{
			case '+':
			case '.':
				stringBuilder.Append(CreateEscapedIdentifier(baseType2.Substring(num, i - num)));
				stringBuilder.Append('.');
				i++;
				num = i;
				break;
			case '`':
			{
				stringBuilder.Append(CreateEscapedIdentifier(baseType2.Substring(num, i - num)));
				i++;
				int num3 = 0;
				for (; i < baseType2.Length && baseType2[i] >= '0' && baseType2[i] <= '9'; i++)
				{
					num3 = num3 * 10 + (baseType2[i] - 48);
				}
				GetTypeArgumentsOutput(typeRef.TypeArguments, num2, num3, stringBuilder);
				num2 += num3;
				if (i < baseType2.Length && (baseType2[i] == '+' || baseType2[i] == '.'))
				{
					stringBuilder.Append('.');
					i++;
				}
				num = i;
				break;
			}
			}
		}
		if (num < baseType2.Length)
		{
			stringBuilder.Append(CreateEscapedIdentifier(baseType2.Substring(num)));
		}
		return stringBuilder.ToString();
	}

	private string GetTypeArgumentsOutput(CodeTypeReferenceCollection typeArguments)
	{
		StringBuilder stringBuilder = new StringBuilder(128);
		GetTypeArgumentsOutput(typeArguments, 0, typeArguments.Count, stringBuilder);
		return stringBuilder.ToString();
	}

	private void GetTypeArgumentsOutput(CodeTypeReferenceCollection typeArguments, int start, int length, StringBuilder sb)
	{
		sb.Append('<');
		bool flag = true;
		for (int i = start; i < start + length; i++)
		{
			if (flag)
			{
				flag = false;
			}
			else
			{
				sb.Append(", ");
			}
			if (i < typeArguments.Count)
			{
				sb.Append(GetTypeOutput(typeArguments[i]));
			}
		}
		sb.Append('>');
	}

	public string GetTypeOutput(CodeTypeReference typeRef)
	{
		string empty = string.Empty;
		CodeTypeReference codeTypeReference = typeRef;
		while (codeTypeReference.ArrayElementType != null)
		{
			codeTypeReference = codeTypeReference.ArrayElementType;
		}
		empty += GetBaseTypeOutput(codeTypeReference);
		while (typeRef != null && typeRef.ArrayRank > 0)
		{
			char[] array = new char[typeRef.ArrayRank + 1];
			array[0] = '[';
			array[typeRef.ArrayRank] = ']';
			for (int i = 1; i < typeRef.ArrayRank; i++)
			{
				array[i] = ',';
			}
			empty += new string(array);
			typeRef = typeRef.ArrayElementType;
		}
		return empty;
	}

	private void OutputStartingBrace()
	{
		if (Options.BracingStyle == "C")
		{
			Output.WriteLine();
			Output.WriteLine('{');
		}
		else
		{
			Output.WriteLine(" {");
		}
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromDom(CompilerParameters options, CodeCompileUnit e)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		try
		{
			return FromDom(options, e);
		}
		finally
		{
			options.TempFiles.SafeDelete();
		}
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromFile(CompilerParameters options, string fileName)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		try
		{
			return FromFile(options, fileName);
		}
		finally
		{
			options.TempFiles.SafeDelete();
		}
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromSource(CompilerParameters options, string source)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		try
		{
			return FromSource(options, source);
		}
		finally
		{
			options.TempFiles.SafeDelete();
		}
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromSourceBatch(CompilerParameters options, string[] sources)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		try
		{
			return FromSourceBatch(options, sources);
		}
		finally
		{
			options.TempFiles.SafeDelete();
		}
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromFileBatch(CompilerParameters options, string[] fileNames)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		if (fileNames == null)
		{
			throw new ArgumentNullException("fileNames");
		}
		try
		{
			for (int i = 0; i < fileNames.Length; i++)
			{
				File.OpenRead(fileNames[i]).Dispose();
			}
			return FromFileBatch(options, fileNames);
		}
		finally
		{
			options.TempFiles.SafeDelete();
		}
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromDomBatch(CompilerParameters options, CodeCompileUnit[] ea)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		try
		{
			return FromDomBatch(options, ea);
		}
		finally
		{
			options.TempFiles.SafeDelete();
		}
	}

	private CompilerResults FromDom(CompilerParameters options, CodeCompileUnit e)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		return FromDomBatch(options, new CodeCompileUnit[1] { e });
	}

	private CompilerResults FromFile(CompilerParameters options, string fileName)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		if (fileName == null)
		{
			throw new ArgumentNullException("fileName");
		}
		File.OpenRead(fileName).Dispose();
		return FromFileBatch(options, new string[1] { fileName });
	}

	private CompilerResults FromSource(CompilerParameters options, string source)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		return FromSourceBatch(options, new string[1] { source });
	}

	private CompilerResults FromDomBatch(CompilerParameters options, CodeCompileUnit[] ea)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		if (ea == null)
		{
			throw new ArgumentNullException("ea");
		}
		string[] array = new string[ea.Length];
		for (int i = 0; i < ea.Length; i++)
		{
			if (ea[i] == null)
			{
				continue;
			}
			ResolveReferencedAssemblies(options, ea[i]);
			array[i] = options.TempFiles.AddExtension(i + FileExtension);
			using FileStream stream = new FileStream(array[i], FileMode.Create, FileAccess.Write, FileShare.Read);
			using StreamWriter streamWriter = new StreamWriter(stream, Encoding.UTF8);
			((ICodeGenerator)this).GenerateCodeFromCompileUnit(ea[i], (TextWriter)streamWriter, Options);
			streamWriter.Flush();
		}
		return FromFileBatch(options, array);
	}

	private void ResolveReferencedAssemblies(CompilerParameters options, CodeCompileUnit e)
	{
		if (e.ReferencedAssemblies.Count <= 0)
		{
			return;
		}
		StringEnumerator enumerator = e.ReferencedAssemblies.GetEnumerator();
		try
		{
			while (enumerator.MoveNext())
			{
				string current = enumerator.Current;
				if (!options.ReferencedAssemblies.Contains(current))
				{
					options.ReferencedAssemblies.Add(current);
				}
			}
		}
		finally
		{
			if (enumerator is IDisposable disposable)
			{
				disposable.Dispose();
			}
		}
	}

	private CompilerResults FromSourceBatch(CompilerParameters options, string[] sources)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		if (sources == null)
		{
			throw new ArgumentNullException("sources");
		}
		string[] array = new string[sources.Length];
		for (int i = 0; i < sources.Length; i++)
		{
			string text = options.TempFiles.AddExtension(i + FileExtension);
			using (FileStream stream = new FileStream(text, FileMode.Create, FileAccess.Write, FileShare.Read))
			{
				using StreamWriter streamWriter = new StreamWriter(stream, Encoding.UTF8);
				streamWriter.Write(sources[i]);
				streamWriter.Flush();
			}
			array[i] = text;
		}
		return FromFileBatch(options, array);
	}

	private static string JoinStringArray(string[] sa, string separator)
	{
		if (sa == null || sa.Length == 0)
		{
			return string.Empty;
		}
		if (sa.Length == 1)
		{
			return "\"" + sa[0] + "\"";
		}
		StringBuilder stringBuilder = new StringBuilder();
		for (int i = 0; i < sa.Length - 1; i++)
		{
			stringBuilder.Append('"');
			stringBuilder.Append(sa[i]);
			stringBuilder.Append('"');
			stringBuilder.Append(separator);
		}
		stringBuilder.Append('"');
		stringBuilder.Append(sa[sa.Length - 1]);
		stringBuilder.Append('"');
		return stringBuilder.ToString();
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

	private CompilerResults FromFileBatch(CompilerParameters options, string[] fileNames)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		if (fileNames == null)
		{
			throw new ArgumentNullException("fileNames");
		}
		CompilerResults results = new CompilerResults(options.TempFiles);
		Process process = new Process();
		if (Path.DirectorySeparatorChar == '\\')
		{
			process.StartInfo.FileName = MonoToolsLocator.Mono;
			process.StartInfo.Arguments = "\"" + MonoToolsLocator.McsCSharpCompiler + "\" ";
		}
		else
		{
			process.StartInfo.FileName = MonoToolsLocator.McsCSharpCompiler;
		}
		process.StartInfo.Arguments += BuildArgs(options, fileNames, _provOptions);
		Mutex mcsOutMutex = new Mutex();
		process.StartInfo.EnvironmentVariables["MONO_GC_PARAMS"] = string.Empty;
		process.StartInfo.CreateNoWindow = true;
		process.StartInfo.UseShellExecute = false;
		process.StartInfo.RedirectStandardOutput = true;
		process.StartInfo.RedirectStandardError = true;
		process.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs args)
		{
			if (args.Data != null)
			{
				mcsOutMutex.WaitOne();
				results.Output.Add(args.Data);
				mcsOutMutex.ReleaseMutex();
			}
		};
		ProcessStartInfo startInfo = process.StartInfo;
		Encoding standardOutputEncoding = (process.StartInfo.StandardErrorEncoding = Encoding.UTF8);
		startInfo.StandardOutputEncoding = standardOutputEncoding;
		try
		{
			process.Start();
		}
		catch (Exception ex)
		{
			if (ex is Win32Exception ex2)
			{
				throw new SystemException($"Error running {process.StartInfo.FileName}: {Win32Exception.GetErrorMessage(ex2.NativeErrorCode)}");
			}
			throw;
		}
		try
		{
			process.BeginOutputReadLine();
			process.BeginErrorReadLine();
			process.WaitForExit();
			results.NativeCompilerReturnValue = process.ExitCode;
		}
		finally
		{
			process.CancelErrorRead();
			process.CancelOutputRead();
			process.Close();
		}
		bool flag = true;
		StringEnumerator enumerator = results.Output.GetEnumerator();
		try
		{
			while (enumerator.MoveNext())
			{
				CompilerError compilerError = CreateErrorFromString(enumerator.Current);
				if (compilerError != null)
				{
					results.Errors.Add(compilerError);
					if (!compilerError.IsWarning)
					{
						flag = false;
					}
				}
			}
		}
		finally
		{
			if (enumerator is IDisposable disposable)
			{
				disposable.Dispose();
			}
		}
		if (results.Output.Count > 0)
		{
			results.Output.Insert(0, process.StartInfo.FileName + " " + process.StartInfo.Arguments + Environment.NewLine);
		}
		if (flag)
		{
			if (!File.Exists(options.OutputAssembly))
			{
				StringBuilder stringBuilder = new StringBuilder();
				enumerator = results.Output.GetEnumerator();
				try
				{
					while (enumerator.MoveNext())
					{
						string current = enumerator.Current;
						stringBuilder.Append(current + Environment.NewLine);
					}
				}
				finally
				{
					if (enumerator is IDisposable disposable2)
					{
						disposable2.Dispose();
					}
				}
				throw new Exception("Compiler failed to produce the assembly. Output: '" + stringBuilder.ToString() + "'");
			}
			if (options.GenerateInMemory)
			{
				using FileStream fileStream = File.OpenRead(options.OutputAssembly);
				byte[] array = new byte[fileStream.Length];
				fileStream.Read(array, 0, array.Length);
				results.CompiledAssembly = Assembly.Load(array, null);
				fileStream.Close();
			}
			else
			{
				results.PathToAssembly = options.OutputAssembly;
			}
		}
		else
		{
			results.CompiledAssembly = null;
		}
		return results;
	}

	private static string BuildArgs(CompilerParameters options, string[] fileNames, IDictionary<string, string> providerOptions)
	{
		StringBuilder stringBuilder = new StringBuilder();
		if (options.GenerateExecutable)
		{
			stringBuilder.Append("/target:exe ");
		}
		else
		{
			stringBuilder.Append("/target:library ");
		}
		string privateBinPath = AppDomain.CurrentDomain.SetupInformation.PrivateBinPath;
		if (privateBinPath != null && privateBinPath.Length > 0)
		{
			stringBuilder.AppendFormat("/lib:\"{0}\" ", privateBinPath);
		}
		if (options.Win32Resource != null)
		{
			stringBuilder.AppendFormat("/win32res:\"{0}\" ", options.Win32Resource);
		}
		if (options.IncludeDebugInformation)
		{
			stringBuilder.Append("/debug+ /optimize- ");
		}
		else
		{
			stringBuilder.Append("/debug- /optimize+ ");
		}
		if (options.TreatWarningsAsErrors)
		{
			stringBuilder.Append("/warnaserror ");
		}
		if (options.WarningLevel >= 0)
		{
			stringBuilder.AppendFormat("/warn:{0} ", options.WarningLevel);
		}
		if (options.OutputAssembly == null || options.OutputAssembly.Length == 0)
		{
			string extension = (options.GenerateExecutable ? "exe" : "dll");
			options.OutputAssembly = GetTempFileNameWithExtension(options.TempFiles, extension, !options.GenerateInMemory);
		}
		stringBuilder.AppendFormat("/out:\"{0}\" ", options.OutputAssembly);
		StringEnumerator enumerator = options.ReferencedAssemblies.GetEnumerator();
		try
		{
			while (enumerator.MoveNext())
			{
				string current = enumerator.Current;
				if (current != null && current.Length != 0)
				{
					stringBuilder.AppendFormat("/r:\"{0}\" ", current);
				}
			}
		}
		finally
		{
			if (enumerator is IDisposable disposable)
			{
				disposable.Dispose();
			}
		}
		if (options.CompilerOptions != null)
		{
			stringBuilder.Append(options.CompilerOptions);
			stringBuilder.Append(" ");
		}
		enumerator = options.EmbeddedResources.GetEnumerator();
		try
		{
			while (enumerator.MoveNext())
			{
				string current2 = enumerator.Current;
				stringBuilder.AppendFormat("/resource:\"{0}\" ", current2);
			}
		}
		finally
		{
			if (enumerator is IDisposable disposable2)
			{
				disposable2.Dispose();
			}
		}
		enumerator = options.LinkedResources.GetEnumerator();
		try
		{
			while (enumerator.MoveNext())
			{
				string current3 = enumerator.Current;
				stringBuilder.AppendFormat("/linkresource:\"{0}\" ", current3);
			}
		}
		finally
		{
			if (enumerator is IDisposable disposable3)
			{
				disposable3.Dispose();
			}
		}
		if (providerOptions != null && providerOptions.Count > 0)
		{
			if (!providerOptions.TryGetValue("CompilerVersion", out var value))
			{
				value = "3.5";
			}
			if (value.Length >= 1 && value[0] == 'v')
			{
				value = value.Substring(1);
			}
			if (!(value == "2.0"))
			{
				if (value == "3.5")
				{
				}
			}
			else
			{
				stringBuilder.Append("/langversion:ISO-2 ");
			}
		}
		stringBuilder.Append("/noconfig ");
		stringBuilder.Append(" -- ");
		foreach (string arg in fileNames)
		{
			stringBuilder.AppendFormat("\"{0}\" ", arg);
		}
		return stringBuilder.ToString();
	}

	private static CompilerError CreateErrorFromString(string error_string)
	{
		if (error_string.StartsWith("BETA"))
		{
			return null;
		}
		if (error_string == null || error_string == "")
		{
			return null;
		}
		CompilerError compilerError = new CompilerError();
		Match match = new Regex("\n\t\t\t^\n\t\t\t(\\s*(?<file>[^\\(]+)                         # filename (optional)\n\t\t\t (\\((?<line>\\d*)(,(?<column>\\d*[\\+]*))?\\))? # line+column (optional)\n\t\t\t :\\s+)?\n\t\t\t(?<level>\\w+)                               # error|warning\n\t\t\t\\s+\n\t\t\t(?<number>[^:]*\\d)                          # CS1234\n\t\t\t:\n\t\t\t\\s*\n\t\t\t(?<message>.*)$", RegexOptions.ExplicitCapture | RegexOptions.Compiled | RegexOptions.IgnorePatternWhitespace).Match(error_string);
		if (!match.Success)
		{
			match = RelatedSymbolsRegex.Match(error_string);
			if (!match.Success)
			{
				compilerError.ErrorText = error_string;
				compilerError.IsWarning = false;
				compilerError.ErrorNumber = "";
				return compilerError;
			}
			return null;
		}
		if (string.Empty != match.Result("${file}"))
		{
			compilerError.FileName = match.Result("${file}");
		}
		if (string.Empty != match.Result("${line}"))
		{
			compilerError.Line = int.Parse(match.Result("${line}"));
		}
		if (string.Empty != match.Result("${column}"))
		{
			compilerError.Column = int.Parse(match.Result("${column}").Trim('+'));
		}
		string text = match.Result("${level}");
		if (text == "warning")
		{
			compilerError.IsWarning = true;
		}
		else if (text != "error")
		{
			return null;
		}
		compilerError.ErrorNumber = match.Result("${number}");
		compilerError.ErrorText = match.Result("${message}");
		return compilerError;
	}

	private static string GetTempFileNameWithExtension(TempFileCollection temp_files, string extension, bool keepFile)
	{
		return temp_files.AddExtension(extension, keepFile);
	}
}
