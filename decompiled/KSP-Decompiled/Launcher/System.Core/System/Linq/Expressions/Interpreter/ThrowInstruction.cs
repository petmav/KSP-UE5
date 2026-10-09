using System.Reflection;
using System.Runtime.CompilerServices;

namespace System.Linq.Expressions.Interpreter;

internal sealed class ThrowInstruction : Instruction
{
	internal static readonly ThrowInstruction Throw = new ThrowInstruction(hasResult: true, isRethrow: false);

	internal static readonly ThrowInstruction VoidThrow = new ThrowInstruction(hasResult: false, isRethrow: false);

	internal static readonly ThrowInstruction Rethrow = new ThrowInstruction(hasResult: true, isRethrow: true);

	internal static readonly ThrowInstruction VoidRethrow = new ThrowInstruction(hasResult: false, isRethrow: true);

	private static ConstructorInfo _runtimeWrappedExceptionCtor;

	private readonly bool _hasResult;

	private readonly bool _rethrow;

	public override string InstructionName => "Throw";

	public override int ProducedStack
	{
		get
		{
			if (!_hasResult)
			{
				return 0;
			}
			return 1;
		}
	}

	public override int ConsumedStack => 1;

	private ThrowInstruction(bool hasResult, bool isRethrow)
	{
		_hasResult = hasResult;
		_rethrow = isRethrow;
	}

	public override int Run(InterpretedFrame frame)
	{
		Exception ex = WrapThrownObject(frame.Pop());
		if (_rethrow)
		{
			throw new RethrowException();
		}
		throw ex;
	}

	private static Exception WrapThrownObject(object thrown)
	{
		object obj;
		if (thrown != null)
		{
			obj = thrown as Exception;
			if (obj == null)
			{
				return RuntimeWrap(thrown);
			}
		}
		else
		{
			obj = null;
		}
		return (Exception)obj;
	}

	private static RuntimeWrappedException RuntimeWrap(object thrown)
	{
		return (RuntimeWrappedException)(_runtimeWrappedExceptionCtor ?? (_runtimeWrappedExceptionCtor = typeof(RuntimeWrappedException).GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.ExactBinding, null, new Type[1] { typeof(object) }, null))).Invoke(new object[1] { thrown });
	}
}
