using System.Runtime.InteropServices;

namespace System.Reflection.Emit;

[ComVisible(true)]
public struct OpCode
{
	internal byte op1;

	internal byte op2;

	private byte push;

	private byte pop;

	private byte size;

	private byte type;

	private byte args;

	private byte flow;

	public string Name
	{
		get
		{
			if (op1 == byte.MaxValue)
			{
				return OpCodeNames.names[op2];
			}
			return OpCodeNames.names[256 + op2];
		}
	}

	public int Size => size;

	public OpCodeType OpCodeType => (OpCodeType)type;

	public OperandType OperandType => (OperandType)args;

	public FlowControl FlowControl => (FlowControl)flow;

	public StackBehaviour StackBehaviourPop => (StackBehaviour)pop;

	public StackBehaviour StackBehaviourPush => (StackBehaviour)push;

	public short Value
	{
		get
		{
			if (size == 1)
			{
				return op2;
			}
			return (short)((op1 << 8) | op2);
		}
	}

	internal OpCode(int p, int q)
	{
		op1 = (byte)((uint)p & 0xFFu);
		op2 = (byte)((uint)(p >> 8) & 0xFFu);
		push = (byte)((uint)(p >> 16) & 0xFFu);
		pop = (byte)((uint)(p >> 24) & 0xFFu);
		size = (byte)((uint)q & 0xFFu);
		type = (byte)((uint)(q >> 8) & 0xFFu);
		args = (byte)((uint)(q >> 16) & 0xFFu);
		flow = (byte)((uint)(q >> 24) & 0xFFu);
	}

	public override int GetHashCode()
	{
		return Name.GetHashCode();
	}

	public override bool Equals(object obj)
	{
		if (obj == null || !(obj is OpCode opCode))
		{
			return false;
		}
		if (opCode.op1 == op1)
		{
			return opCode.op2 == op2;
		}
		return false;
	}

	public bool Equals(OpCode obj)
	{
		if (obj.op1 == op1)
		{
			return obj.op2 == op2;
		}
		return false;
	}

	public override string ToString()
	{
		return Name;
	}

	public static bool operator ==(OpCode a, OpCode b)
	{
		if (a.op1 == b.op1)
		{
			return a.op2 == b.op2;
		}
		return false;
	}

	public static bool operator !=(OpCode a, OpCode b)
	{
		if (a.op1 == b.op1)
		{
			return a.op2 != b.op2;
		}
		return true;
	}
}
