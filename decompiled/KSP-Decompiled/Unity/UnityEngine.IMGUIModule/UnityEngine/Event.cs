using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine;

[StructLayout(LayoutKind.Sequential)]
[StaticAccessor("GUIEvent", StaticAccessorType.DoubleColon)]
[NativeHeader("Modules/IMGUI/Event.bindings.h")]
public sealed class Event
{
	[NonSerialized]
	internal IntPtr m_Ptr;

	private static Event s_Current;

	private static Event s_MasterEvent;

	[NativeProperty("type", false, TargetType.Field)]
	public extern EventType rawType
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	[NativeProperty("mousePosition", false, TargetType.Field)]
	public Vector2 mousePosition
	{
		get
		{
			get_mousePosition_Injected(out var ret);
			return ret;
		}
		set
		{
			set_mousePosition_Injected(ref value);
		}
	}

	[NativeProperty("delta", false, TargetType.Field)]
	public Vector2 delta
	{
		get
		{
			get_delta_Injected(out var ret);
			return ret;
		}
		set
		{
			set_delta_Injected(ref value);
		}
	}

	[NativeProperty("pointerType", false, TargetType.Field)]
	public extern PointerType pointerType
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("button", false, TargetType.Field)]
	public extern int button
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("modifiers", false, TargetType.Field)]
	public extern EventModifiers modifiers
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("pressure", false, TargetType.Field)]
	public extern float pressure
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("clickCount", false, TargetType.Field)]
	public extern int clickCount
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("character", false, TargetType.Field)]
	public extern char character
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("keycode", false, TargetType.Field)]
	public extern KeyCode keyCode
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("displayIndex", false, TargetType.Field)]
	public extern int displayIndex
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern EventType type
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[FreeFunction("GUIEvent::GetType", HasExplicitThis = true)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[FreeFunction("GUIEvent::SetType", HasExplicitThis = true)]
		set;
	}

	public extern string commandName
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[FreeFunction("GUIEvent::GetCommandName", HasExplicitThis = true)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[FreeFunction("GUIEvent::SetCommandName", HasExplicitThis = true)]
		set;
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	[Obsolete("Use HandleUtility.GUIPointToWorldRay(Event.current.mousePosition);", true)]
	public Ray mouseRay
	{
		get
		{
			return new Ray(Vector3.up, Vector3.up);
		}
		set
		{
		}
	}

	public bool shift
	{
		get
		{
			return (modifiers & EventModifiers.Shift) != 0;
		}
		set
		{
			if (!value)
			{
				modifiers &= ~EventModifiers.Shift;
			}
			else
			{
				modifiers |= EventModifiers.Shift;
			}
		}
	}

	public bool control
	{
		get
		{
			return (modifiers & EventModifiers.Control) != 0;
		}
		set
		{
			if (!value)
			{
				modifiers &= ~EventModifiers.Control;
			}
			else
			{
				modifiers |= EventModifiers.Control;
			}
		}
	}

	public bool alt
	{
		get
		{
			return (modifiers & EventModifiers.Alt) != 0;
		}
		set
		{
			if (!value)
			{
				modifiers &= ~EventModifiers.Alt;
			}
			else
			{
				modifiers |= EventModifiers.Alt;
			}
		}
	}

	public bool command
	{
		get
		{
			return (modifiers & EventModifiers.Command) != 0;
		}
		set
		{
			if (!value)
			{
				modifiers &= ~EventModifiers.Command;
			}
			else
			{
				modifiers |= EventModifiers.Command;
			}
		}
	}

	public bool capsLock
	{
		get
		{
			return (modifiers & EventModifiers.CapsLock) != 0;
		}
		set
		{
			if (!value)
			{
				modifiers &= ~EventModifiers.CapsLock;
			}
			else
			{
				modifiers |= EventModifiers.CapsLock;
			}
		}
	}

	public bool numeric
	{
		get
		{
			return (modifiers & EventModifiers.Numeric) != 0;
		}
		set
		{
			if (!value)
			{
				modifiers &= ~EventModifiers.Numeric;
			}
			else
			{
				modifiers |= EventModifiers.Numeric;
			}
		}
	}

	public bool functionKey => (modifiers & EventModifiers.FunctionKey) != 0;

	public static Event current
	{
		get
		{
			return s_Current;
		}
		set
		{
			s_Current = value ?? s_MasterEvent;
			Internal_SetNativeEvent(s_Current.m_Ptr);
		}
	}

	public bool isKey
	{
		get
		{
			EventType eventType = type;
			return eventType == EventType.KeyDown || eventType == EventType.KeyUp;
		}
	}

	public bool isMouse
	{
		get
		{
			EventType eventType = type;
			return eventType == EventType.MouseMove || eventType == EventType.MouseDown || eventType == EventType.MouseUp || eventType == EventType.MouseDrag || eventType == EventType.ContextClick || eventType == EventType.MouseEnterWindow || eventType == EventType.MouseLeaveWindow;
		}
	}

	public bool isScrollWheel
	{
		get
		{
			EventType eventType = type;
			return eventType == EventType.ScrollWheel;
		}
	}

	internal bool isDirectManipulationDevice
	{
		[VisibleToOtherModules(new string[] { "UnityEngine.UIElementsModule" })]
		get
		{
			return pointerType == PointerType.Pen || pointerType == PointerType.Touch;
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("Use")]
	private extern void Internal_Use();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("GUIEvent::Internal_Create", IsThreadSafe = true)]
	private static extern IntPtr Internal_Create(int displayIndex);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("GUIEvent::Internal_Destroy", IsThreadSafe = true)]
	private static extern void Internal_Destroy(IntPtr ptr);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("GUIEvent::Internal_Copy", IsThreadSafe = true)]
	private static extern IntPtr Internal_Copy(IntPtr otherPtr);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("GUIEvent::GetTypeForControl", HasExplicitThis = true)]
	public extern EventType GetTypeForControl(int controlID);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[VisibleToOtherModules(new string[] { "UnityEngine.UIElementsModule" })]
	[FreeFunction("GUIEvent::CopyFromPtr", IsThreadSafe = true, HasExplicitThis = true)]
	internal extern void CopyFromPtr(IntPtr ptr);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern bool PopEvent([NotNull] Event outEvent);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern int GetEventCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Internal_SetNativeEvent(IntPtr ptr);

	[RequiredByNativeCode]
	internal static void Internal_MakeMasterEventCurrent(int displayIndex)
	{
		if (s_MasterEvent == null)
		{
			s_MasterEvent = new Event(displayIndex);
		}
		s_MasterEvent.displayIndex = displayIndex;
		s_Current = s_MasterEvent;
		Internal_SetNativeEvent(s_MasterEvent.m_Ptr);
	}

	public Event()
	{
		m_Ptr = Internal_Create(0);
	}

	public Event(int displayIndex)
	{
		m_Ptr = Internal_Create(displayIndex);
	}

	public Event(Event other)
	{
		if (other == null)
		{
			throw new ArgumentException("Event to copy from is null.");
		}
		m_Ptr = Internal_Copy(other.m_Ptr);
	}

	~Event()
	{
		if (m_Ptr != IntPtr.Zero)
		{
			Internal_Destroy(m_Ptr);
			m_Ptr = IntPtr.Zero;
		}
	}

	internal static void CleanupRoots()
	{
		s_Current = null;
		s_MasterEvent = null;
	}

	[VisibleToOtherModules(new string[] { "UnityEngine.UIElementsModule" })]
	internal void CopyFrom(Event e)
	{
		if (e.m_Ptr != m_Ptr)
		{
			CopyFromPtr(e.m_Ptr);
		}
	}

	public static Event KeyboardEvent(string key)
	{
		Event @event = new Event(0)
		{
			type = EventType.KeyDown
		};
		if (string.IsNullOrEmpty(key))
		{
			return @event;
		}
		int num = 0;
		bool flag = false;
		do
		{
			flag = true;
			if (num >= key.Length)
			{
				flag = false;
				break;
			}
			switch (key[num])
			{
			case '&':
				@event.modifiers |= EventModifiers.Alt;
				num++;
				break;
			case '^':
				@event.modifiers |= EventModifiers.Control;
				num++;
				break;
			case '%':
				@event.modifiers |= EventModifiers.Command;
				num++;
				break;
			case '#':
				@event.modifiers |= EventModifiers.Shift;
				num++;
				break;
			default:
				flag = false;
				break;
			}
		}
		while (flag);
		string text = key.Substring(num, key.Length - num).ToLowerInvariant();
		switch (text)
		{
		case "[0]":
			@event.character = '0';
			@event.keyCode = KeyCode.Keypad0;
			break;
		case "[1]":
			@event.character = '1';
			@event.keyCode = KeyCode.Keypad1;
			break;
		case "[2]":
			@event.character = '2';
			@event.keyCode = KeyCode.Keypad2;
			break;
		case "[3]":
			@event.character = '3';
			@event.keyCode = KeyCode.Keypad3;
			break;
		case "[4]":
			@event.character = '4';
			@event.keyCode = KeyCode.Keypad4;
			break;
		case "[5]":
			@event.character = '5';
			@event.keyCode = KeyCode.Keypad5;
			break;
		case "[6]":
			@event.character = '6';
			@event.keyCode = KeyCode.Keypad6;
			break;
		case "[7]":
			@event.character = '7';
			@event.keyCode = KeyCode.Keypad7;
			break;
		case "[8]":
			@event.character = '8';
			@event.keyCode = KeyCode.Keypad8;
			break;
		case "[9]":
			@event.character = '9';
			@event.keyCode = KeyCode.Keypad9;
			break;
		case "[.]":
			@event.character = '.';
			@event.keyCode = KeyCode.KeypadPeriod;
			break;
		case "[/]":
			@event.character = '/';
			@event.keyCode = KeyCode.KeypadDivide;
			break;
		case "[-]":
			@event.character = '-';
			@event.keyCode = KeyCode.KeypadMinus;
			break;
		case "[+]":
			@event.character = '+';
			@event.keyCode = KeyCode.KeypadPlus;
			break;
		case "[=]":
			@event.character = '=';
			@event.keyCode = KeyCode.KeypadEquals;
			break;
		case "[equals]":
			@event.character = '=';
			@event.keyCode = KeyCode.KeypadEquals;
			break;
		case "[enter]":
			@event.character = '\n';
			@event.keyCode = KeyCode.KeypadEnter;
			break;
		case "up":
			@event.keyCode = KeyCode.UpArrow;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "down":
			@event.keyCode = KeyCode.DownArrow;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "left":
			@event.keyCode = KeyCode.LeftArrow;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "right":
			@event.keyCode = KeyCode.RightArrow;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "insert":
			@event.keyCode = KeyCode.Insert;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "home":
			@event.keyCode = KeyCode.Home;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "end":
			@event.keyCode = KeyCode.End;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "pgup":
			@event.keyCode = KeyCode.PageDown;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "page up":
			@event.keyCode = KeyCode.PageUp;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "pgdown":
			@event.keyCode = KeyCode.PageUp;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "page down":
			@event.keyCode = KeyCode.PageDown;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "backspace":
			@event.keyCode = KeyCode.Backspace;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "delete":
			@event.keyCode = KeyCode.Delete;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "tab":
			@event.keyCode = KeyCode.Tab;
			break;
		case "f1":
			@event.keyCode = KeyCode.F1;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f2":
			@event.keyCode = KeyCode.F2;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f3":
			@event.keyCode = KeyCode.F3;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f4":
			@event.keyCode = KeyCode.F4;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f5":
			@event.keyCode = KeyCode.F5;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f6":
			@event.keyCode = KeyCode.F6;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f7":
			@event.keyCode = KeyCode.F7;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f8":
			@event.keyCode = KeyCode.F8;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f9":
			@event.keyCode = KeyCode.F9;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f10":
			@event.keyCode = KeyCode.F10;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f11":
			@event.keyCode = KeyCode.F11;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f12":
			@event.keyCode = KeyCode.F12;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f13":
			@event.keyCode = KeyCode.F13;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f14":
			@event.keyCode = KeyCode.F14;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "f15":
			@event.keyCode = KeyCode.F15;
			@event.modifiers |= EventModifiers.FunctionKey;
			break;
		case "[esc]":
			@event.keyCode = KeyCode.Escape;
			break;
		case "return":
			@event.character = '\n';
			@event.keyCode = KeyCode.Return;
			@event.modifiers &= ~EventModifiers.FunctionKey;
			break;
		case "space":
			@event.keyCode = KeyCode.Space;
			@event.character = ' ';
			@event.modifiers &= ~EventModifiers.FunctionKey;
			break;
		default:
			if (text.Length != 1)
			{
				try
				{
					@event.keyCode = (KeyCode)Enum.Parse(typeof(KeyCode), text, ignoreCase: true);
				}
				catch (ArgumentException)
				{
					Debug.LogError(UnityString.Format("Unable to find key name that matches '{0}'", text));
				}
			}
			else
			{
				@event.character = text.ToLower()[0];
				@event.keyCode = (KeyCode)@event.character;
				if (@event.modifiers != 0)
				{
					@event.character = '\0';
				}
			}
			break;
		}
		return @event;
	}

	public override int GetHashCode()
	{
		int num = 1;
		if (isKey)
		{
			num = (ushort)keyCode;
		}
		if (isMouse)
		{
			num = mousePosition.GetHashCode();
		}
		return (num * 37) | (int)modifiers;
	}

	public override bool Equals(object obj)
	{
		if (obj == null)
		{
			return false;
		}
		if (this == obj)
		{
			return true;
		}
		if ((object)obj.GetType() != GetType())
		{
			return false;
		}
		Event @event = (Event)obj;
		if (type != @event.type || (modifiers & ~EventModifiers.CapsLock) != (@event.modifiers & ~EventModifiers.CapsLock))
		{
			return false;
		}
		if (isKey)
		{
			return keyCode == @event.keyCode;
		}
		if (isMouse)
		{
			return mousePosition == @event.mousePosition;
		}
		return false;
	}

	public override string ToString()
	{
		if (isKey)
		{
			if (character == '\0')
			{
				return UnityString.Format("Event:{0}   Character:\\0   Modifiers:{1}   KeyCode:{2}", type, modifiers, keyCode);
			}
			return string.Concat("Event:", type, "   Character:", (int)character, "   Modifiers:", modifiers, "   KeyCode:", keyCode);
		}
		if (isMouse)
		{
			return UnityString.Format("Event: {0}   Position: {1} Modifiers: {2}", type, mousePosition, modifiers);
		}
		if (type == EventType.ExecuteCommand || type == EventType.ValidateCommand)
		{
			return UnityString.Format("Event: {0}  \"{1}\"", type, commandName);
		}
		return string.Concat(type);
	}

	public void Use()
	{
		if (type == EventType.Repaint || type == EventType.Layout)
		{
			Debug.LogWarning(UnityString.Format("Event.Use() should not be called for events of type {0}", type));
		}
		Internal_Use();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_mousePosition_Injected(out Vector2 ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_mousePosition_Injected(ref Vector2 value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_delta_Injected(out Vector2 ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_delta_Injected(ref Vector2 value);
}
