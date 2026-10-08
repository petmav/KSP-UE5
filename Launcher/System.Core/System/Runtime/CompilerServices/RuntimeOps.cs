using System.ComponentModel;
using System.Diagnostics;
using System.Dynamic;
using System.Linq.Expressions;
using System.Linq.Expressions.Compiler;
using Unity;

namespace System.Runtime.CompilerServices;

[EditorBrowsable(EditorBrowsableState.Never)]
[DebuggerStepThrough]
public static class RuntimeOps
{
	internal sealed class MergedRuntimeVariables : IRuntimeVariables
	{
		private readonly IRuntimeVariables _first;

		private readonly IRuntimeVariables _second;

		private readonly int[] _indexes;

		public int Count => _indexes.Length;

		public object this[int index]
		{
			get
			{
				index = _indexes[index];
				if (index < 0)
				{
					return _second[-1 - index];
				}
				return _first[index];
			}
			set
			{
				index = _indexes[index];
				if (index >= 0)
				{
					_first[index] = value;
				}
				else
				{
					_second[-1 - index] = value;
				}
			}
		}

		internal MergedRuntimeVariables(IRuntimeVariables first, IRuntimeVariables second, int[] indexes)
		{
			_first = first;
			_second = second;
			_indexes = indexes;
		}
	}

	private sealed class EmptyRuntimeVariables : IRuntimeVariables
	{
		int IRuntimeVariables.Count => 0;

		object IRuntimeVariables.this[int index]
		{
			get
			{
				throw new IndexOutOfRangeException();
			}
			set
			{
				throw new IndexOutOfRangeException();
			}
		}
	}

	private sealed class RuntimeVariableList : IRuntimeVariables
	{
		private readonly object[] _data;

		private readonly long[] _indexes;

		public int Count => _indexes.Length;

		public object this[int index]
		{
			get
			{
				return GetStrongBox(index).Value;
			}
			set
			{
				GetStrongBox(index).Value = value;
			}
		}

		internal RuntimeVariableList(object[] data, long[] indexes)
		{
			_data = data;
			_indexes = indexes;
		}

		private IStrongBox GetStrongBox(int index)
		{
			long num = _indexes[index];
			object[] array = _data;
			for (int num2 = (int)(num >> 32); num2 > 0; num2--)
			{
				array = HoistedLocals.GetParent(array);
			}
			return (IStrongBox)array[(int)num];
		}
	}

	internal sealed class RuntimeVariables : IRuntimeVariables
	{
		private readonly IStrongBox[] _boxes;

		int IRuntimeVariables.Count => _boxes.Length;

		object IRuntimeVariables.this[int index]
		{
			get
			{
				return _boxes[index].Value;
			}
			set
			{
				_boxes[index].Value = value;
			}
		}

		internal RuntimeVariables(IStrongBox[] boxes)
		{
			_boxes = boxes;
		}
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool ExpandoTryGetValue(ExpandoObject expando, object indexClass, int index, string name, bool ignoreCase, out object value)
	{
		return expando.TryGetValue(indexClass, index, name, ignoreCase, out value);
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static object ExpandoTrySetValue(ExpandoObject expando, object indexClass, int index, object value, string name, bool ignoreCase)
	{
		expando.TrySetValue(indexClass, index, value, name, ignoreCase, add: false);
		return value;
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool ExpandoTryDeleteValue(ExpandoObject expando, object indexClass, int index, string name, bool ignoreCase)
	{
		return expando.TryDeleteValue(indexClass, index, name, ignoreCase, ExpandoObject.Uninitialized);
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static bool ExpandoCheckVersion(ExpandoObject expando, object version)
	{
		return expando.Class == version;
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static void ExpandoPromoteClass(ExpandoObject expando, object oldClass, object newClass)
	{
		expando.PromoteClass(oldClass, newClass);
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static IRuntimeVariables CreateRuntimeVariables(object[] data, long[] indexes)
	{
		return new RuntimeVariableList(data, indexes);
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static IRuntimeVariables CreateRuntimeVariables()
	{
		return new EmptyRuntimeVariables();
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static IRuntimeVariables MergeRuntimeVariables(IRuntimeVariables first, IRuntimeVariables second, int[] indexes)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("do not use this method", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public static Expression Quote(Expression expression, object hoistedLocals, object[] locals)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
