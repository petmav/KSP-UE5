using System.Runtime.Interop;
using System.Security;
using System.Security.Permissions;

namespace System.Runtime.Diagnostics;

internal sealed class EtwProvider : DiagnosticsEventProvider
{
	private Action invokeControllerCallback;

	private bool end2EndActivityTracingEnabled;

	internal Action ControllerCallBack
	{
		get
		{
			return invokeControllerCallback;
		}
		set
		{
			invokeControllerCallback = value;
		}
	}

	internal bool IsEnd2EndActivityTracingEnabled => end2EndActivityTracingEnabled;

	[SecurityCritical]
	[PermissionSet(SecurityAction.Assert, Unrestricted = true)]
	internal EtwProvider(Guid id)
		: base(id)
	{
	}

	protected override void OnControllerCommand()
	{
		end2EndActivityTracingEnabled = false;
		if (invokeControllerCallback != null)
		{
			invokeControllerCallback();
		}
	}

	internal void SetEnd2EndActivityTracingEnabled(bool isEnd2EndActivityTracingEnabled)
	{
		end2EndActivityTracingEnabled = isEnd2EndActivityTracingEnabled;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, string value2, string value3)
	{
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value2)
		{
			fixed (char* ptr4 = value3)
			{
				byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 3)];
				UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
				ptr2->DataPointer = (ulong)(&value1);
				ptr2->Size = (uint)sizeof(Guid);
				ptr2[1].DataPointer = (ulong)ptr3;
				ptr2[1].Size = (uint)((value2.Length + 1) * 2);
				ptr2[2].DataPointer = (ulong)ptr4;
				ptr2[2].Size = (uint)((value3.Length + 1) * 2);
				result = WriteEvent(ref eventDescriptor, eventTraceActivity, 3, (IntPtr)ptr);
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteTransferEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid relatedActivityId, string value1, string value2)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 2)];
				UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
				ptr2->DataPointer = (ulong)ptr3;
				ptr2->Size = (uint)((value1.Length + 1) * 2);
				ptr2[1].DataPointer = (ulong)ptr4;
				ptr2[1].Size = (uint)((value2.Length + 1) * 2);
				result = WriteTransferEvent(ref eventDescriptor, eventTraceActivity, relatedActivityId, 2, (IntPtr)ptr);
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 2)];
				UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
				ptr2->DataPointer = (ulong)ptr3;
				ptr2->Size = (uint)((value1.Length + 1) * 2);
				ptr2[1].DataPointer = (ulong)ptr4;
				ptr2[1].Size = (uint)((value2.Length + 1) * 2);
				result = WriteEvent(ref eventDescriptor, eventTraceActivity, 2, (IntPtr)ptr);
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 3)];
					UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
					ptr2->DataPointer = (ulong)ptr3;
					ptr2->Size = (uint)((value1.Length + 1) * 2);
					ptr2[1].DataPointer = (ulong)ptr4;
					ptr2[1].Size = (uint)((value2.Length + 1) * 2);
					ptr2[2].DataPointer = (ulong)ptr5;
					ptr2[2].Size = (uint)((value3.Length + 1) * 2);
					result = WriteEvent(ref eventDescriptor, eventTraceActivity, 3, (IntPtr)ptr);
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 4)];
						UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
						ptr2->DataPointer = (ulong)ptr3;
						ptr2->Size = (uint)((value1.Length + 1) * 2);
						ptr2[1].DataPointer = (ulong)ptr4;
						ptr2[1].Size = (uint)((value2.Length + 1) * 2);
						ptr2[2].DataPointer = (ulong)ptr5;
						ptr2[2].Size = (uint)((value3.Length + 1) * 2);
						ptr2[3].DataPointer = (ulong)ptr6;
						ptr2[3].Size = (uint)((value4.Length + 1) * 2);
						result = WriteEvent(ref eventDescriptor, eventTraceActivity, 4, (IntPtr)ptr);
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 5)];
							UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
							ptr2->DataPointer = (ulong)ptr3;
							ptr2->Size = (uint)((value1.Length + 1) * 2);
							ptr2[1].DataPointer = (ulong)ptr4;
							ptr2[1].Size = (uint)((value2.Length + 1) * 2);
							ptr2[2].DataPointer = (ulong)ptr5;
							ptr2[2].Size = (uint)((value3.Length + 1) * 2);
							ptr2[3].DataPointer = (ulong)ptr6;
							ptr2[3].Size = (uint)((value4.Length + 1) * 2);
							ptr2[4].DataPointer = (ulong)ptr7;
							ptr2[4].Size = (uint)((value5.Length + 1) * 2);
							result = WriteEvent(ref eventDescriptor, eventTraceActivity, 5, (IntPtr)ptr);
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 6)];
								UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
								ptr2->DataPointer = (ulong)ptr3;
								ptr2->Size = (uint)((value1.Length + 1) * 2);
								ptr2[1].DataPointer = (ulong)ptr4;
								ptr2[1].Size = (uint)((value2.Length + 1) * 2);
								ptr2[2].DataPointer = (ulong)ptr5;
								ptr2[2].Size = (uint)((value3.Length + 1) * 2);
								ptr2[3].DataPointer = (ulong)ptr6;
								ptr2[3].Size = (uint)((value4.Length + 1) * 2);
								ptr2[4].DataPointer = (ulong)ptr7;
								ptr2[4].Size = (uint)((value5.Length + 1) * 2);
								ptr2[5].DataPointer = (ulong)ptr8;
								ptr2[5].Size = (uint)((value6.Length + 1) * 2);
								result = WriteEvent(ref eventDescriptor, eventTraceActivity, 6, (IntPtr)ptr);
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 7)];
									UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
									ptr2->DataPointer = (ulong)ptr3;
									ptr2->Size = (uint)((value1.Length + 1) * 2);
									ptr2[1].DataPointer = (ulong)ptr4;
									ptr2[1].Size = (uint)((value2.Length + 1) * 2);
									ptr2[2].DataPointer = (ulong)ptr5;
									ptr2[2].Size = (uint)((value3.Length + 1) * 2);
									ptr2[3].DataPointer = (ulong)ptr6;
									ptr2[3].Size = (uint)((value4.Length + 1) * 2);
									ptr2[4].DataPointer = (ulong)ptr7;
									ptr2[4].Size = (uint)((value5.Length + 1) * 2);
									ptr2[5].DataPointer = (ulong)ptr8;
									ptr2[5].Size = (uint)((value6.Length + 1) * 2);
									ptr2[6].DataPointer = (ulong)ptr9;
									ptr2[6].Size = (uint)((value7.Length + 1) * 2);
									result = WriteEvent(ref eventDescriptor, eventTraceActivity, 7, (IntPtr)ptr);
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7, string value8)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									fixed (char* ptr10 = value8)
									{
										byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 8)];
										UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
										ptr2->DataPointer = (ulong)ptr3;
										ptr2->Size = (uint)((value1.Length + 1) * 2);
										ptr2[1].DataPointer = (ulong)ptr4;
										ptr2[1].Size = (uint)((value2.Length + 1) * 2);
										ptr2[2].DataPointer = (ulong)ptr5;
										ptr2[2].Size = (uint)((value3.Length + 1) * 2);
										ptr2[3].DataPointer = (ulong)ptr6;
										ptr2[3].Size = (uint)((value4.Length + 1) * 2);
										ptr2[4].DataPointer = (ulong)ptr7;
										ptr2[4].Size = (uint)((value5.Length + 1) * 2);
										ptr2[5].DataPointer = (ulong)ptr8;
										ptr2[5].Size = (uint)((value6.Length + 1) * 2);
										ptr2[6].DataPointer = (ulong)ptr9;
										ptr2[6].Size = (uint)((value7.Length + 1) * 2);
										ptr2[7].DataPointer = (ulong)ptr10;
										ptr2[7].Size = (uint)((value8.Length + 1) * 2);
										result = WriteEvent(ref eventDescriptor, eventTraceActivity, 8, (IntPtr)ptr);
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7, string value8, string value9)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									fixed (char* ptr10 = value8)
									{
										fixed (char* ptr11 = value9)
										{
											byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 9)];
											UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
											ptr2->DataPointer = (ulong)ptr3;
											ptr2->Size = (uint)((value1.Length + 1) * 2);
											ptr2[1].DataPointer = (ulong)ptr4;
											ptr2[1].Size = (uint)((value2.Length + 1) * 2);
											ptr2[2].DataPointer = (ulong)ptr5;
											ptr2[2].Size = (uint)((value3.Length + 1) * 2);
											ptr2[3].DataPointer = (ulong)ptr6;
											ptr2[3].Size = (uint)((value4.Length + 1) * 2);
											ptr2[4].DataPointer = (ulong)ptr7;
											ptr2[4].Size = (uint)((value5.Length + 1) * 2);
											ptr2[5].DataPointer = (ulong)ptr8;
											ptr2[5].Size = (uint)((value6.Length + 1) * 2);
											ptr2[6].DataPointer = (ulong)ptr9;
											ptr2[6].Size = (uint)((value7.Length + 1) * 2);
											ptr2[7].DataPointer = (ulong)ptr10;
											ptr2[7].Size = (uint)((value8.Length + 1) * 2);
											ptr2[8].DataPointer = (ulong)ptr11;
											ptr2[8].Size = (uint)((value9.Length + 1) * 2);
											result = WriteEvent(ref eventDescriptor, eventTraceActivity, 9, (IntPtr)ptr);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									fixed (char* ptr10 = value8)
									{
										fixed (char* ptr11 = value9)
										{
											fixed (char* ptr12 = value10)
											{
												byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 10)];
												UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
												ptr2->DataPointer = (ulong)ptr3;
												ptr2->Size = (uint)((value1.Length + 1) * 2);
												ptr2[1].DataPointer = (ulong)ptr4;
												ptr2[1].Size = (uint)((value2.Length + 1) * 2);
												ptr2[2].DataPointer = (ulong)ptr5;
												ptr2[2].Size = (uint)((value3.Length + 1) * 2);
												ptr2[3].DataPointer = (ulong)ptr6;
												ptr2[3].Size = (uint)((value4.Length + 1) * 2);
												ptr2[4].DataPointer = (ulong)ptr7;
												ptr2[4].Size = (uint)((value5.Length + 1) * 2);
												ptr2[5].DataPointer = (ulong)ptr8;
												ptr2[5].Size = (uint)((value6.Length + 1) * 2);
												ptr2[6].DataPointer = (ulong)ptr9;
												ptr2[6].Size = (uint)((value7.Length + 1) * 2);
												ptr2[7].DataPointer = (ulong)ptr10;
												ptr2[7].Size = (uint)((value8.Length + 1) * 2);
												ptr2[8].DataPointer = (ulong)ptr11;
												ptr2[8].Size = (uint)((value9.Length + 1) * 2);
												ptr2[9].DataPointer = (ulong)ptr12;
												ptr2[9].Size = (uint)((value10.Length + 1) * 2);
												result = WriteEvent(ref eventDescriptor, eventTraceActivity, 10, (IntPtr)ptr);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									fixed (char* ptr10 = value8)
									{
										fixed (char* ptr11 = value9)
										{
											fixed (char* ptr12 = value10)
											{
												fixed (char* ptr13 = value11)
												{
													byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 11)];
													UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
													ptr2->DataPointer = (ulong)ptr3;
													ptr2->Size = (uint)((value1.Length + 1) * 2);
													ptr2[1].DataPointer = (ulong)ptr4;
													ptr2[1].Size = (uint)((value2.Length + 1) * 2);
													ptr2[2].DataPointer = (ulong)ptr5;
													ptr2[2].Size = (uint)((value3.Length + 1) * 2);
													ptr2[3].DataPointer = (ulong)ptr6;
													ptr2[3].Size = (uint)((value4.Length + 1) * 2);
													ptr2[4].DataPointer = (ulong)ptr7;
													ptr2[4].Size = (uint)((value5.Length + 1) * 2);
													ptr2[5].DataPointer = (ulong)ptr8;
													ptr2[5].Size = (uint)((value6.Length + 1) * 2);
													ptr2[6].DataPointer = (ulong)ptr9;
													ptr2[6].Size = (uint)((value7.Length + 1) * 2);
													ptr2[7].DataPointer = (ulong)ptr10;
													ptr2[7].Size = (uint)((value8.Length + 1) * 2);
													ptr2[8].DataPointer = (ulong)ptr11;
													ptr2[8].Size = (uint)((value9.Length + 1) * 2);
													ptr2[9].DataPointer = (ulong)ptr12;
													ptr2[9].Size = (uint)((value10.Length + 1) * 2);
													ptr2[10].DataPointer = (ulong)ptr13;
													ptr2[10].Size = (uint)((value11.Length + 1) * 2);
													result = WriteEvent(ref eventDescriptor, eventTraceActivity, 11, (IntPtr)ptr);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									fixed (char* ptr10 = value8)
									{
										fixed (char* ptr11 = value9)
										{
											fixed (char* ptr12 = value10)
											{
												fixed (char* ptr13 = value11)
												{
													fixed (char* ptr14 = value12)
													{
														byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 12)];
														UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
														ptr2->DataPointer = (ulong)ptr3;
														ptr2->Size = (uint)((value1.Length + 1) * 2);
														ptr2[1].DataPointer = (ulong)ptr4;
														ptr2[1].Size = (uint)((value2.Length + 1) * 2);
														ptr2[2].DataPointer = (ulong)ptr5;
														ptr2[2].Size = (uint)((value3.Length + 1) * 2);
														ptr2[3].DataPointer = (ulong)ptr6;
														ptr2[3].Size = (uint)((value4.Length + 1) * 2);
														ptr2[4].DataPointer = (ulong)ptr7;
														ptr2[4].Size = (uint)((value5.Length + 1) * 2);
														ptr2[5].DataPointer = (ulong)ptr8;
														ptr2[5].Size = (uint)((value6.Length + 1) * 2);
														ptr2[6].DataPointer = (ulong)ptr9;
														ptr2[6].Size = (uint)((value7.Length + 1) * 2);
														ptr2[7].DataPointer = (ulong)ptr10;
														ptr2[7].Size = (uint)((value8.Length + 1) * 2);
														ptr2[8].DataPointer = (ulong)ptr11;
														ptr2[8].Size = (uint)((value9.Length + 1) * 2);
														ptr2[9].DataPointer = (ulong)ptr12;
														ptr2[9].Size = (uint)((value10.Length + 1) * 2);
														ptr2[10].DataPointer = (ulong)ptr13;
														ptr2[10].Size = (uint)((value11.Length + 1) * 2);
														ptr2[11].DataPointer = (ulong)ptr14;
														ptr2[11].Size = (uint)((value12.Length + 1) * 2);
														result = WriteEvent(ref eventDescriptor, eventTraceActivity, 12, (IntPtr)ptr);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, string value2, string value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12, string value13)
	{
		value1 = value1 ?? string.Empty;
		value2 = value2 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		value13 = value13 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value2)
			{
				fixed (char* ptr5 = value3)
				{
					fixed (char* ptr6 = value4)
					{
						fixed (char* ptr7 = value5)
						{
							fixed (char* ptr8 = value6)
							{
								fixed (char* ptr9 = value7)
								{
									fixed (char* ptr10 = value8)
									{
										fixed (char* ptr11 = value9)
										{
											fixed (char* ptr12 = value10)
											{
												fixed (char* ptr13 = value11)
												{
													fixed (char* ptr14 = value12)
													{
														fixed (char* ptr15 = value13)
														{
															byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 13)];
															UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
															ptr2->DataPointer = (ulong)ptr3;
															ptr2->Size = (uint)((value1.Length + 1) * 2);
															ptr2[1].DataPointer = (ulong)ptr4;
															ptr2[1].Size = (uint)((value2.Length + 1) * 2);
															ptr2[2].DataPointer = (ulong)ptr5;
															ptr2[2].Size = (uint)((value3.Length + 1) * 2);
															ptr2[3].DataPointer = (ulong)ptr6;
															ptr2[3].Size = (uint)((value4.Length + 1) * 2);
															ptr2[4].DataPointer = (ulong)ptr7;
															ptr2[4].Size = (uint)((value5.Length + 1) * 2);
															ptr2[5].DataPointer = (ulong)ptr8;
															ptr2[5].Size = (uint)((value6.Length + 1) * 2);
															ptr2[6].DataPointer = (ulong)ptr9;
															ptr2[6].Size = (uint)((value7.Length + 1) * 2);
															ptr2[7].DataPointer = (ulong)ptr10;
															ptr2[7].Size = (uint)((value8.Length + 1) * 2);
															ptr2[8].DataPointer = (ulong)ptr11;
															ptr2[8].Size = (uint)((value9.Length + 1) * 2);
															ptr2[9].DataPointer = (ulong)ptr12;
															ptr2[9].Size = (uint)((value10.Length + 1) * 2);
															ptr2[10].DataPointer = (ulong)ptr13;
															ptr2[10].Size = (uint)((value11.Length + 1) * 2);
															ptr2[11].DataPointer = (ulong)ptr14;
															ptr2[11].Size = (uint)((value12.Length + 1) * 2);
															ptr2[12].DataPointer = (ulong)ptr15;
															ptr2[12].Size = (uint)((value13.Length + 1) * 2);
															result = WriteEvent(ref eventDescriptor, eventTraceActivity, 13, (IntPtr)ptr);
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, int value1)
	{
		byte* ptr = stackalloc byte[(int)(uint)sizeof(UnsafeNativeMethods.EventData)];
		UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
		ptr2->DataPointer = (ulong)(&value1);
		ptr2->Size = 4u;
		return WriteEvent(ref eventDescriptor, eventTraceActivity, 1, (IntPtr)ptr);
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, int value1, int value2)
	{
		byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 2)];
		UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
		ptr2->DataPointer = (ulong)(&value1);
		ptr2->Size = 4u;
		ptr2[1].DataPointer = (ulong)(&value2);
		ptr2[1].Size = 4u;
		return WriteEvent(ref eventDescriptor, eventTraceActivity, 2, (IntPtr)ptr);
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, int value1, int value2, int value3)
	{
		byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 3)];
		UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
		ptr2->DataPointer = (ulong)(&value1);
		ptr2->Size = 4u;
		ptr2[1].DataPointer = (ulong)(&value2);
		ptr2[1].Size = 4u;
		ptr2[2].DataPointer = (ulong)(&value3);
		ptr2[2].Size = 4u;
		return WriteEvent(ref eventDescriptor, eventTraceActivity, 3, (IntPtr)ptr);
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, long value1)
	{
		byte* ptr = stackalloc byte[(int)(uint)sizeof(UnsafeNativeMethods.EventData)];
		UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
		ptr2->DataPointer = (ulong)(&value1);
		ptr2->Size = 8u;
		return WriteEvent(ref eventDescriptor, eventTraceActivity, 1, (IntPtr)ptr);
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, long value1, long value2)
	{
		byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 2)];
		UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
		ptr2->DataPointer = (ulong)(&value1);
		ptr2->Size = 8u;
		ptr2[1].DataPointer = (ulong)(&value2);
		ptr2[1].Size = 8u;
		return WriteEvent(ref eventDescriptor, eventTraceActivity, 2, (IntPtr)ptr);
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, long value1, long value2, long value3)
	{
		byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 3)];
		UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
		ptr2->DataPointer = (ulong)(&value1);
		ptr2->Size = 8u;
		ptr2[1].DataPointer = (ulong)(&value2);
		ptr2[1].Size = 8u;
		ptr2[2].DataPointer = (ulong)(&value3);
		ptr2[2].Size = 8u;
		return WriteEvent(ref eventDescriptor, eventTraceActivity, 3, (IntPtr)ptr);
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12, string value13, string value14, string value15)
	{
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		value13 = value13 ?? string.Empty;
		value14 = value14 ?? string.Empty;
		value15 = value15 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value5)
			{
				fixed (char* ptr5 = value6)
				{
					fixed (char* ptr6 = value7)
					{
						fixed (char* ptr7 = value8)
						{
							fixed (char* ptr8 = value9)
							{
								fixed (char* ptr9 = value10)
								{
									fixed (char* ptr10 = value11)
									{
										fixed (char* ptr11 = value12)
										{
											fixed (char* ptr12 = value13)
											{
												fixed (char* ptr13 = value14)
												{
													fixed (char* ptr14 = value15)
													{
														byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 15)];
														UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
														ptr2->DataPointer = (ulong)(&value1);
														ptr2->Size = (uint)sizeof(Guid);
														ptr2[1].DataPointer = (ulong)(&value2);
														ptr2[1].Size = 8u;
														ptr2[2].DataPointer = (ulong)(&value3);
														ptr2[2].Size = 8u;
														ptr2[3].DataPointer = (ulong)ptr3;
														ptr2[3].Size = (uint)((value4.Length + 1) * 2);
														ptr2[4].DataPointer = (ulong)ptr4;
														ptr2[4].Size = (uint)((value5.Length + 1) * 2);
														ptr2[5].DataPointer = (ulong)ptr5;
														ptr2[5].Size = (uint)((value6.Length + 1) * 2);
														ptr2[6].DataPointer = (ulong)ptr6;
														ptr2[6].Size = (uint)((value7.Length + 1) * 2);
														ptr2[7].DataPointer = (ulong)ptr7;
														ptr2[7].Size = (uint)((value8.Length + 1) * 2);
														ptr2[8].DataPointer = (ulong)ptr8;
														ptr2[8].Size = (uint)((value9.Length + 1) * 2);
														ptr2[9].DataPointer = (ulong)ptr9;
														ptr2[9].Size = (uint)((value10.Length + 1) * 2);
														ptr2[10].DataPointer = (ulong)ptr10;
														ptr2[10].Size = (uint)((value11.Length + 1) * 2);
														ptr2[11].DataPointer = (ulong)ptr11;
														ptr2[11].Size = (uint)((value12.Length + 1) * 2);
														ptr2[12].DataPointer = (ulong)ptr12;
														ptr2[12].Size = (uint)((value13.Length + 1) * 2);
														ptr2[13].DataPointer = (ulong)ptr13;
														ptr2[13].Size = (uint)((value14.Length + 1) * 2);
														ptr2[14].DataPointer = (ulong)ptr14;
														ptr2[14].Size = (uint)((value15.Length + 1) * 2);
														result = WriteEvent(ref eventDescriptor, eventTraceActivity, 15, (IntPtr)ptr);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12, bool value13, string value14, string value15, string value16, string value17)
	{
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		value14 = value14 ?? string.Empty;
		value15 = value15 ?? string.Empty;
		value16 = value16 ?? string.Empty;
		value17 = value17 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value5)
			{
				fixed (char* ptr5 = value6)
				{
					fixed (char* ptr6 = value7)
					{
						fixed (char* ptr7 = value8)
						{
							fixed (char* ptr8 = value9)
							{
								fixed (char* ptr9 = value10)
								{
									fixed (char* ptr10 = value11)
									{
										fixed (char* ptr11 = value12)
										{
											fixed (char* ptr12 = value14)
											{
												fixed (char* ptr13 = value15)
												{
													fixed (char* ptr14 = value16)
													{
														fixed (char* ptr15 = value17)
														{
															byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 17)];
															UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
															ptr2->DataPointer = (ulong)(&value1);
															ptr2->Size = (uint)sizeof(Guid);
															ptr2[1].DataPointer = (ulong)(&value2);
															ptr2[1].Size = 8u;
															ptr2[2].DataPointer = (ulong)(&value3);
															ptr2[2].Size = 8u;
															ptr2[3].DataPointer = (ulong)ptr3;
															ptr2[3].Size = (uint)((value4.Length + 1) * 2);
															ptr2[4].DataPointer = (ulong)ptr4;
															ptr2[4].Size = (uint)((value5.Length + 1) * 2);
															ptr2[5].DataPointer = (ulong)ptr5;
															ptr2[5].Size = (uint)((value6.Length + 1) * 2);
															ptr2[6].DataPointer = (ulong)ptr6;
															ptr2[6].Size = (uint)((value7.Length + 1) * 2);
															ptr2[7].DataPointer = (ulong)ptr7;
															ptr2[7].Size = (uint)((value8.Length + 1) * 2);
															ptr2[8].DataPointer = (ulong)ptr8;
															ptr2[8].Size = (uint)((value9.Length + 1) * 2);
															ptr2[9].DataPointer = (ulong)ptr9;
															ptr2[9].Size = (uint)((value10.Length + 1) * 2);
															ptr2[10].DataPointer = (ulong)ptr10;
															ptr2[10].Size = (uint)((value11.Length + 1) * 2);
															ptr2[11].DataPointer = (ulong)ptr11;
															ptr2[11].Size = (uint)((value12.Length + 1) * 2);
															ptr2[12].DataPointer = (ulong)(&value13);
															ptr2[12].Size = 1u;
															ptr2[13].DataPointer = (ulong)ptr12;
															ptr2[13].Size = (uint)((value14.Length + 1) * 2);
															ptr2[14].DataPointer = (ulong)ptr13;
															ptr2[14].Size = (uint)((value15.Length + 1) * 2);
															ptr2[15].DataPointer = (ulong)ptr14;
															ptr2[15].Size = (uint)((value16.Length + 1) * 2);
															ptr2[16].DataPointer = (ulong)ptr15;
															ptr2[16].Size = (uint)((value17.Length + 1) * 2);
															result = WriteEvent(ref eventDescriptor, eventTraceActivity, 17, (IntPtr)ptr);
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, string value5, string value6, string value7, string value8, string value9)
	{
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value5)
			{
				fixed (char* ptr5 = value6)
				{
					fixed (char* ptr6 = value7)
					{
						fixed (char* ptr7 = value8)
						{
							fixed (char* ptr8 = value9)
							{
								byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 9)];
								UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
								ptr2->DataPointer = (ulong)(&value1);
								ptr2->Size = (uint)sizeof(Guid);
								ptr2[1].DataPointer = (ulong)(&value2);
								ptr2[1].Size = 8u;
								ptr2[2].DataPointer = (ulong)(&value3);
								ptr2[2].Size = 8u;
								ptr2[3].DataPointer = (ulong)ptr3;
								ptr2[3].Size = (uint)((value4.Length + 1) * 2);
								ptr2[4].DataPointer = (ulong)ptr4;
								ptr2[4].Size = (uint)((value5.Length + 1) * 2);
								ptr2[5].DataPointer = (ulong)ptr5;
								ptr2[5].Size = (uint)((value6.Length + 1) * 2);
								ptr2[6].DataPointer = (ulong)ptr6;
								ptr2[6].Size = (uint)((value7.Length + 1) * 2);
								ptr2[7].DataPointer = (ulong)ptr7;
								ptr2[7].Size = (uint)((value8.Length + 1) * 2);
								ptr2[8].DataPointer = (ulong)ptr8;
								ptr2[8].Size = (uint)((value9.Length + 1) * 2);
								result = WriteEvent(ref eventDescriptor, eventTraceActivity, 9, (IntPtr)ptr);
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11)
	{
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value5)
			{
				fixed (char* ptr5 = value6)
				{
					fixed (char* ptr6 = value7)
					{
						fixed (char* ptr7 = value8)
						{
							fixed (char* ptr8 = value9)
							{
								fixed (char* ptr9 = value10)
								{
									fixed (char* ptr10 = value11)
									{
										byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 11)];
										UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
										ptr2->DataPointer = (ulong)(&value1);
										ptr2->Size = (uint)sizeof(Guid);
										ptr2[1].DataPointer = (ulong)(&value2);
										ptr2[1].Size = 8u;
										ptr2[2].DataPointer = (ulong)(&value3);
										ptr2[2].Size = 8u;
										ptr2[3].DataPointer = (ulong)ptr3;
										ptr2[3].Size = (uint)((value4.Length + 1) * 2);
										ptr2[4].DataPointer = (ulong)ptr4;
										ptr2[4].Size = (uint)((value5.Length + 1) * 2);
										ptr2[5].DataPointer = (ulong)ptr5;
										ptr2[5].Size = (uint)((value6.Length + 1) * 2);
										ptr2[6].DataPointer = (ulong)ptr6;
										ptr2[6].Size = (uint)((value7.Length + 1) * 2);
										ptr2[7].DataPointer = (ulong)ptr7;
										ptr2[7].Size = (uint)((value8.Length + 1) * 2);
										ptr2[8].DataPointer = (ulong)ptr8;
										ptr2[8].Size = (uint)((value9.Length + 1) * 2);
										ptr2[9].DataPointer = (ulong)ptr9;
										ptr2[9].Size = (uint)((value10.Length + 1) * 2);
										ptr2[10].DataPointer = (ulong)ptr10;
										ptr2[10].Size = (uint)((value11.Length + 1) * 2);
										result = WriteEvent(ref eventDescriptor, eventTraceActivity, 11, (IntPtr)ptr);
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12, string value13)
	{
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		value13 = value13 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value5)
			{
				fixed (char* ptr5 = value6)
				{
					fixed (char* ptr6 = value7)
					{
						fixed (char* ptr7 = value8)
						{
							fixed (char* ptr8 = value9)
							{
								fixed (char* ptr9 = value10)
								{
									fixed (char* ptr10 = value11)
									{
										fixed (char* ptr11 = value12)
										{
											fixed (char* ptr12 = value13)
											{
												byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 13)];
												UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
												ptr2->DataPointer = (ulong)(&value1);
												ptr2->Size = (uint)sizeof(Guid);
												ptr2[1].DataPointer = (ulong)(&value2);
												ptr2[1].Size = 8u;
												ptr2[2].DataPointer = (ulong)(&value3);
												ptr2[2].Size = 8u;
												ptr2[3].DataPointer = (ulong)ptr3;
												ptr2[3].Size = (uint)((value4.Length + 1) * 2);
												ptr2[4].DataPointer = (ulong)ptr4;
												ptr2[4].Size = (uint)((value5.Length + 1) * 2);
												ptr2[5].DataPointer = (ulong)ptr5;
												ptr2[5].Size = (uint)((value6.Length + 1) * 2);
												ptr2[6].DataPointer = (ulong)ptr6;
												ptr2[6].Size = (uint)((value7.Length + 1) * 2);
												ptr2[7].DataPointer = (ulong)ptr7;
												ptr2[7].Size = (uint)((value8.Length + 1) * 2);
												ptr2[8].DataPointer = (ulong)ptr8;
												ptr2[8].Size = (uint)((value9.Length + 1) * 2);
												ptr2[9].DataPointer = (ulong)ptr9;
												ptr2[9].Size = (uint)((value10.Length + 1) * 2);
												ptr2[10].DataPointer = (ulong)ptr10;
												ptr2[10].Size = (uint)((value11.Length + 1) * 2);
												ptr2[11].DataPointer = (ulong)ptr11;
												ptr2[11].Size = (uint)((value12.Length + 1) * 2);
												ptr2[12].DataPointer = (ulong)ptr12;
												ptr2[12].Size = (uint)((value13.Length + 1) * 2);
												result = WriteEvent(ref eventDescriptor, eventTraceActivity, 13, (IntPtr)ptr);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, string value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12, string value13, string value14)
	{
		value4 = value4 ?? string.Empty;
		value5 = value5 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		value13 = value13 ?? string.Empty;
		value14 = value14 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value5)
			{
				fixed (char* ptr5 = value6)
				{
					fixed (char* ptr6 = value7)
					{
						fixed (char* ptr7 = value8)
						{
							fixed (char* ptr8 = value9)
							{
								fixed (char* ptr9 = value10)
								{
									fixed (char* ptr10 = value11)
									{
										fixed (char* ptr11 = value12)
										{
											fixed (char* ptr12 = value13)
											{
												fixed (char* ptr13 = value14)
												{
													byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 14)];
													UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
													ptr2->DataPointer = (ulong)(&value1);
													ptr2->Size = (uint)sizeof(Guid);
													ptr2[1].DataPointer = (ulong)(&value2);
													ptr2[1].Size = 8u;
													ptr2[2].DataPointer = (ulong)(&value3);
													ptr2[2].Size = 8u;
													ptr2[3].DataPointer = (ulong)ptr3;
													ptr2[3].Size = (uint)((value4.Length + 1) * 2);
													ptr2[4].DataPointer = (ulong)ptr4;
													ptr2[4].Size = (uint)((value5.Length + 1) * 2);
													ptr2[5].DataPointer = (ulong)ptr5;
													ptr2[5].Size = (uint)((value6.Length + 1) * 2);
													ptr2[6].DataPointer = (ulong)ptr6;
													ptr2[6].Size = (uint)((value7.Length + 1) * 2);
													ptr2[7].DataPointer = (ulong)ptr7;
													ptr2[7].Size = (uint)((value8.Length + 1) * 2);
													ptr2[8].DataPointer = (ulong)ptr8;
													ptr2[8].Size = (uint)((value9.Length + 1) * 2);
													ptr2[9].DataPointer = (ulong)ptr9;
													ptr2[9].Size = (uint)((value10.Length + 1) * 2);
													ptr2[10].DataPointer = (ulong)ptr10;
													ptr2[10].Size = (uint)((value11.Length + 1) * 2);
													ptr2[11].DataPointer = (ulong)ptr11;
													ptr2[11].Size = (uint)((value12.Length + 1) * 2);
													ptr2[12].DataPointer = (ulong)ptr12;
													ptr2[12].Size = (uint)((value13.Length + 1) * 2);
													ptr2[13].DataPointer = (ulong)ptr13;
													ptr2[13].Size = (uint)((value14.Length + 1) * 2);
													result = WriteEvent(ref eventDescriptor, eventTraceActivity, 14, (IntPtr)ptr);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, Guid value1, long value2, long value3, string value4, Guid value5, string value6, string value7, string value8, string value9, string value10, string value11, string value12, string value13)
	{
		value4 = value4 ?? string.Empty;
		value6 = value6 ?? string.Empty;
		value7 = value7 ?? string.Empty;
		value8 = value8 ?? string.Empty;
		value9 = value9 ?? string.Empty;
		value10 = value10 ?? string.Empty;
		value11 = value11 ?? string.Empty;
		value12 = value12 ?? string.Empty;
		value13 = value13 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value4)
		{
			fixed (char* ptr4 = value6)
			{
				fixed (char* ptr5 = value7)
				{
					fixed (char* ptr6 = value8)
					{
						fixed (char* ptr7 = value9)
						{
							fixed (char* ptr8 = value10)
							{
								fixed (char* ptr9 = value11)
								{
									fixed (char* ptr10 = value12)
									{
										fixed (char* ptr11 = value13)
										{
											byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 13)];
											UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
											ptr2->DataPointer = (ulong)(&value1);
											ptr2->Size = (uint)sizeof(Guid);
											ptr2[1].DataPointer = (ulong)(&value2);
											ptr2[1].Size = 8u;
											ptr2[2].DataPointer = (ulong)(&value3);
											ptr2[2].Size = 8u;
											ptr2[3].DataPointer = (ulong)ptr3;
											ptr2[3].Size = (uint)((value4.Length + 1) * 2);
											ptr2[4].DataPointer = (ulong)(&value5);
											ptr2[4].Size = (uint)sizeof(Guid);
											ptr2[5].DataPointer = (ulong)ptr4;
											ptr2[5].Size = (uint)((value6.Length + 1) * 2);
											ptr2[6].DataPointer = (ulong)ptr5;
											ptr2[6].Size = (uint)((value7.Length + 1) * 2);
											ptr2[7].DataPointer = (ulong)ptr6;
											ptr2[7].Size = (uint)((value8.Length + 1) * 2);
											ptr2[8].DataPointer = (ulong)ptr7;
											ptr2[8].Size = (uint)((value9.Length + 1) * 2);
											ptr2[9].DataPointer = (ulong)ptr8;
											ptr2[9].Size = (uint)((value10.Length + 1) * 2);
											ptr2[10].DataPointer = (ulong)ptr9;
											ptr2[10].Size = (uint)((value11.Length + 1) * 2);
											ptr2[11].DataPointer = (ulong)ptr10;
											ptr2[11].Size = (uint)((value12.Length + 1) * 2);
											ptr2[12].DataPointer = (ulong)ptr11;
											ptr2[12].Size = (uint)((value13.Length + 1) * 2);
											result = WriteEvent(ref eventDescriptor, eventTraceActivity, 13, (IntPtr)ptr);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	[SecurityCritical]
	internal unsafe bool WriteEvent(ref EventDescriptor eventDescriptor, EventTraceActivity eventTraceActivity, string value1, long value2, string value3, string value4)
	{
		value1 = value1 ?? string.Empty;
		value3 = value3 ?? string.Empty;
		value4 = value4 ?? string.Empty;
		bool result;
		fixed (char* ptr3 = value1)
		{
			fixed (char* ptr4 = value3)
			{
				fixed (char* ptr5 = value4)
				{
					byte* ptr = stackalloc byte[(int)(uint)(sizeof(UnsafeNativeMethods.EventData) * 4)];
					UnsafeNativeMethods.EventData* ptr2 = (UnsafeNativeMethods.EventData*)ptr;
					ptr2->DataPointer = (ulong)ptr3;
					ptr2->Size = (uint)((value1.Length + 1) * 2);
					ptr2[1].DataPointer = (ulong)(&value2);
					ptr2[1].Size = 8u;
					ptr2[2].DataPointer = (ulong)ptr4;
					ptr2[2].Size = (uint)((value3.Length + 1) * 2);
					ptr2[3].DataPointer = (ulong)ptr5;
					ptr2[3].Size = (uint)((value4.Length + 1) * 2);
					result = WriteEvent(ref eventDescriptor, eventTraceActivity, 4, (IntPtr)ptr);
				}
			}
		}
		return result;
	}
}
