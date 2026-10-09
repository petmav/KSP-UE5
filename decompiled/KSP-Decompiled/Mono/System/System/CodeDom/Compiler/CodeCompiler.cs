using System.Collections.Specialized;
using System.IO;
using System.Text;

namespace System.CodeDom.Compiler;

public abstract class CodeCompiler : CodeGenerator, ICodeCompiler
{
	protected abstract string FileExtension { get; }

	protected abstract string CompilerName { get; }

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

	protected virtual CompilerResults FromDom(CompilerParameters options, CodeCompileUnit e)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		return FromDomBatch(options, new CodeCompileUnit[1] { e });
	}

	protected virtual CompilerResults FromFile(CompilerParameters options, string fileName)
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

	protected virtual CompilerResults FromSource(CompilerParameters options, string source)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		return FromSourceBatch(options, new string[1] { source });
	}

	protected virtual CompilerResults FromDomBatch(CompilerParameters options, CodeCompileUnit[] ea)
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
			((ICodeGenerator)this).GenerateCodeFromCompileUnit(ea[i], (TextWriter)streamWriter, base.Options);
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

	protected virtual CompilerResults FromFileBatch(CompilerParameters options, string[] fileNames)
	{
		if (options == null)
		{
			throw new ArgumentNullException("options");
		}
		if (fileNames == null)
		{
			throw new ArgumentNullException("fileNames");
		}
		throw new PlatformNotSupportedException();
	}

	protected abstract void ProcessCompilerOutputLine(CompilerResults results, string line);

	protected abstract string CmdArgsFromParameters(CompilerParameters options);

	protected virtual string GetResponseFileCmdArgs(CompilerParameters options, string cmdArgs)
	{
		string text = options.TempFiles.AddExtension("cmdline");
		using (FileStream stream = new FileStream(text, FileMode.Create, FileAccess.Write, FileShare.Read))
		{
			using StreamWriter streamWriter = new StreamWriter(stream, Encoding.UTF8);
			streamWriter.Write(cmdArgs);
			streamWriter.Flush();
		}
		return "@\"" + text + "\"";
	}

	protected virtual CompilerResults FromSourceBatch(CompilerParameters options, string[] sources)
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

	protected static string JoinStringArray(string[] sa, string separator)
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
}
