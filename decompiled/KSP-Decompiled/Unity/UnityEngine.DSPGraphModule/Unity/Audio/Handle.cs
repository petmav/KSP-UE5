using System;
using Unity.Collections.LowLevel.Unsafe;
using UnityEngine.Bindings;

namespace Unity.Audio;

[NativeType(Header = "Modules/DSPGraph/Public/DSPGraphHandles.h")]
internal struct Handle : IHandle<Handle>, IValidatable, IEquatable<Handle>
{
	internal struct Node
	{
		public long Next;

		public int Id;

		public int Version;

		public int DidAllocate;

		public const int InvalidId = -1;
	}

	[NativeDisableUnsafePtrRestriction]
	private IntPtr m_Node;

	public int Version;

	public unsafe Node* AtomicNode
	{
		get
		{
			return (Node*)(void*)m_Node;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException();
			}
			m_Node = (IntPtr)value;
			Version = value->Version;
		}
	}

	public unsafe int Id
	{
		get
		{
			return Valid ? AtomicNode->Id : (-1);
		}
		set
		{
			if (value == -1)
			{
				throw new ArgumentException("Invalid ID");
			}
			if (!Valid)
			{
				throw new InvalidOperationException("Handle is invalid or has been destroyed");
			}
			if (AtomicNode->Id != -1)
			{
				throw new InvalidOperationException($"Trying to overwrite id on live node {AtomicNode->Id}");
			}
			AtomicNode->Id = value;
		}
	}

	public unsafe bool Valid => m_Node != IntPtr.Zero && AtomicNode->Version == Version;

	public unsafe bool Alive => Valid && AtomicNode->Id != -1;

	public unsafe Handle(Node* node)
	{
		if (node == null)
		{
			throw new ArgumentNullException("node");
		}
		if (node->Id != -1)
		{
			throw new InvalidOperationException($"Reusing unflushed node {node->Id}");
		}
		Version = node->Version;
		m_Node = (IntPtr)node;
	}

	public unsafe void FlushNode()
	{
		if (!Valid)
		{
			throw new InvalidOperationException("Attempting to flush invalid audio handle");
		}
		AtomicNode->Id = -1;
		AtomicNode->Version++;
	}

	public bool Equals(Handle other)
	{
		return m_Node == other.m_Node && Version == other.Version;
	}

	public override bool Equals(object obj)
	{
		if (obj == null)
		{
			return false;
		}
		return obj is Handle && Equals((Handle)obj);
	}

	public override int GetHashCode()
	{
		return ((int)m_Node * 397) ^ Version;
	}
}
