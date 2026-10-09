using System;
using Smooth.Collections;
using Smooth.Delegates;

namespace Smooth.Algebraics
{
	[Serializable]
	public struct Either<L, R> : IComparable<Either<L, R>>, IEquatable<Either<L, R>>
	{
		public readonly bool isLeft;

		public readonly L leftValue;

		public readonly R rightValue;

		public bool isRight => !isLeft;

		public Option<L> leftOption
		{
			get
			{
				if (!isLeft)
				{
					return default(Option<L>);
				}
				return new Option<L>(leftValue);
			}
		}

		public Option<R> rightOption
		{
			get
			{
				if (!isLeft)
				{
					return new Option<R>(rightValue);
				}
				return default(Option<R>);
			}
		}

		private Either(bool isLeft, L leftValue, R rightValue)
		{
			this.isLeft = isLeft;
			this.leftValue = leftValue;
			this.rightValue = rightValue;
		}

		public static Either<L, R> Left(L value)
		{
			return new Either<L, R>(isLeft: true, value, default(R));
		}

		public static Either<L, R> Right(R value)
		{
			return new Either<L, R>(isLeft: false, default(L), value);
		}

		public V Cata<V>(DelegateFunc<L, V> leftFunc, DelegateFunc<R, V> rightFunc)
		{
			if (!isLeft)
			{
				return rightFunc(rightValue);
			}
			return leftFunc(leftValue);
		}

		public V Cata<V, P>(DelegateFunc<L, P, V> leftFunc, P p, DelegateFunc<R, V> rightFunc)
		{
			if (!isLeft)
			{
				return rightFunc(rightValue);
			}
			return leftFunc(leftValue, p);
		}

		public V Cata<V, P2>(DelegateFunc<L, V> leftFunc, DelegateFunc<R, P2, V> rightFunc, P2 p2)
		{
			if (!isLeft)
			{
				return rightFunc(rightValue, p2);
			}
			return leftFunc(leftValue);
		}

		public V Cata<V, P, P2>(DelegateFunc<L, P, V> leftFunc, P p, DelegateFunc<R, P2, V> rightFunc, P2 p2)
		{
			if (!isLeft)
			{
				return rightFunc(rightValue, p2);
			}
			return leftFunc(leftValue, p);
		}

		public void ForEach(DelegateAction<L> leftAction, DelegateAction<R> rightAction)
		{
			if (isLeft)
			{
				leftAction(leftValue);
			}
			else
			{
				rightAction(rightValue);
			}
		}

		public void ForEach<P>(DelegateAction<L, P> leftAction, P p, DelegateAction<R> rightAction)
		{
			if (isLeft)
			{
				leftAction(leftValue, p);
			}
			else
			{
				rightAction(rightValue);
			}
		}

		public void ForEach<P2>(DelegateAction<L> leftAction, DelegateAction<R, P2> rightAction, P2 p2)
		{
			if (isLeft)
			{
				leftAction(leftValue);
			}
			else
			{
				rightAction(rightValue, p2);
			}
		}

		public void ForEach<P, P2>(DelegateAction<L, P> leftAction, P p, DelegateAction<R, P2> rightAction, P2 p2)
		{
			if (isLeft)
			{
				leftAction(leftValue, p);
			}
			else
			{
				rightAction(rightValue, p2);
			}
		}

		public override bool Equals(object o)
		{
			if (o is Either<L, R>)
			{
				return Equals((Either<L, R>)o);
			}
			return false;
		}

		public bool Equals(Either<L, R> other)
		{
			if (!isLeft)
			{
				if (!other.isLeft)
				{
					return EqualityComparer<R>.Default.Equals(rightValue, other.rightValue);
				}
				return false;
			}
			if (other.isLeft)
			{
				return EqualityComparer<L>.Default.Equals(leftValue, other.leftValue);
			}
			return false;
		}

		public override int GetHashCode()
		{
			if (!isLeft)
			{
				return EqualityComparer<R>.Default.GetHashCode(rightValue);
			}
			return EqualityComparer<L>.Default.GetHashCode(leftValue);
		}

		public int CompareTo(Either<L, R> other)
		{
			if (!isLeft)
			{
				if (!other.isLeft)
				{
					return Comparer<R>.Default.Compare(rightValue, other.rightValue);
				}
				return 1;
			}
			if (!other.isLeft)
			{
				return -1;
			}
			return Comparer<L>.Default.Compare(leftValue, other.leftValue);
		}

		public static bool operator ==(Either<L, R> lhs, Either<L, R> rhs)
		{
			return lhs.Equals(rhs);
		}

		public static bool operator !=(Either<L, R> lhs, Either<L, R> rhs)
		{
			return !lhs.Equals(rhs);
		}

		public static bool operator >(Either<L, R> lhs, Either<L, R> rhs)
		{
			return lhs.CompareTo(rhs) > 0;
		}

		public static bool operator <(Either<L, R> lhs, Either<L, R> rhs)
		{
			return lhs.CompareTo(rhs) < 0;
		}

		public static bool operator >=(Either<L, R> lhs, Either<L, R> rhs)
		{
			return lhs.CompareTo(rhs) >= 0;
		}

		public static bool operator <=(Either<L, R> lhs, Either<L, R> rhs)
		{
			return lhs.CompareTo(rhs) <= 0;
		}

		public override string ToString()
		{
			if (!isLeft)
			{
				return string.Concat("[Right: ", rightValue, " ]");
			}
			return string.Concat("[Left: ", leftValue, " ]");
		}
	}
}
