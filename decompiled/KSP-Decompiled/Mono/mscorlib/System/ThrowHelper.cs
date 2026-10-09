using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

internal static class ThrowHelper
{
	internal static void ThrowArgumentNullException(ExceptionArgument argument)
	{
		throw CreateArgumentNullException(argument);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateArgumentNullException(ExceptionArgument argument)
	{
		return new ArgumentNullException(argument.ToString());
	}

	internal static void ThrowArrayTypeMismatchException_ArrayTypeMustBeExactMatch(Type type)
	{
		throw CreateArrayTypeMismatchException_ArrayTypeMustBeExactMatch(type);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateArrayTypeMismatchException_ArrayTypeMustBeExactMatch(Type type)
	{
		return new ArrayTypeMismatchException(SR.Format("The array type must be exactly {0}.", type));
	}

	internal static void ThrowArgumentException_InvalidTypeWithPointersNotSupported(Type type)
	{
		throw CreateArgumentException_InvalidTypeWithPointersNotSupported(type);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateArgumentException_InvalidTypeWithPointersNotSupported(Type type)
	{
		return new ArgumentException(SR.Format("Cannot use type '{0}'. Only value types without pointers or references are supported.", type));
	}

	internal static void ThrowArgumentException_DestinationTooShort()
	{
		throw CreateArgumentException_DestinationTooShort();
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateArgumentException_DestinationTooShort()
	{
		return new ArgumentException("Destination is too short.");
	}

	internal static void ThrowIndexOutOfRangeException()
	{
		throw CreateIndexOutOfRangeException();
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateIndexOutOfRangeException()
	{
		return new IndexOutOfRangeException();
	}

	internal static void ThrowArgumentOutOfRangeException(ExceptionArgument argument)
	{
		throw CreateArgumentOutOfRangeException(argument);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateArgumentOutOfRangeException(ExceptionArgument argument)
	{
		return new ArgumentOutOfRangeException(argument.ToString());
	}

	internal static void ThrowInvalidOperationException_OutstandingReferences()
	{
		throw CreateInvalidOperationException_OutstandingReferences();
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateInvalidOperationException_OutstandingReferences()
	{
		return new InvalidOperationException("Release all references before disposing this instance.");
	}

	internal static void ThrowObjectDisposedException_MemoryDisposed(string objectName)
	{
		throw CreateObjectDisposedException_MemoryDisposed(objectName);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static Exception CreateObjectDisposedException_MemoryDisposed(string objectName)
	{
		return new ObjectDisposedException(objectName, "Memory<T> has been disposed.");
	}

	internal static void ThrowArgumentOutOfRangeException()
	{
		ThrowArgumentOutOfRangeException(ExceptionArgument.index, ExceptionResource.ArgumentOutOfRange_Index);
	}

	internal static void ThrowWrongKeyTypeArgumentException(object key, Type targetType)
	{
		throw new ArgumentException(Environment.GetResourceString("The value \"{0}\" is not of type \"{1}\" and cannot be used in this generic collection.", key, targetType), "key");
	}

	internal static void ThrowWrongValueTypeArgumentException(object value, Type targetType)
	{
		throw new ArgumentException(Environment.GetResourceString("The value \"{0}\" is not of type \"{1}\" and cannot be used in this generic collection.", value, targetType), "value");
	}

	internal static void ThrowKeyNotFoundException()
	{
		throw new KeyNotFoundException();
	}

	internal static void ThrowArgumentException(ExceptionResource resource)
	{
		throw new ArgumentException(Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowArgumentException(ExceptionResource resource, ExceptionArgument argument)
	{
		throw new ArgumentException(Environment.GetResourceString(GetResourceName(resource)), GetArgumentName(argument));
	}

	internal static void ThrowArgumentOutOfRangeException(ExceptionArgument argument, ExceptionResource resource)
	{
		if (CompatibilitySwitches.IsAppEarlierThanWindowsPhone8)
		{
			throw new ArgumentOutOfRangeException(GetArgumentName(argument), string.Empty);
		}
		throw new ArgumentOutOfRangeException(GetArgumentName(argument), Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowInvalidOperationException(ExceptionResource resource)
	{
		throw new InvalidOperationException(Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowSerializationException(ExceptionResource resource)
	{
		throw new SerializationException(Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowSecurityException(ExceptionResource resource)
	{
		throw new SecurityException(Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowNotSupportedException(ExceptionResource resource)
	{
		throw new NotSupportedException(Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowUnauthorizedAccessException(ExceptionResource resource)
	{
		throw new UnauthorizedAccessException(Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void ThrowObjectDisposedException(string objectName, ExceptionResource resource)
	{
		throw new ObjectDisposedException(objectName, Environment.GetResourceString(GetResourceName(resource)));
	}

	internal static void IfNullAndNullsAreIllegalThenThrow<T>(object value, ExceptionArgument argName)
	{
		if (value == null && default(T) != null)
		{
			ThrowArgumentNullException(argName);
		}
	}

	internal static string GetArgumentName(ExceptionArgument argument)
	{
		string text = null;
		return argument switch
		{
			ExceptionArgument.array => "array", 
			ExceptionArgument.arrayIndex => "arrayIndex", 
			ExceptionArgument.capacity => "capacity", 
			ExceptionArgument.collection => "collection", 
			ExceptionArgument.list => "list", 
			ExceptionArgument.converter => "converter", 
			ExceptionArgument.count => "count", 
			ExceptionArgument.dictionary => "dictionary", 
			ExceptionArgument.dictionaryCreationThreshold => "dictionaryCreationThreshold", 
			ExceptionArgument.index => "index", 
			ExceptionArgument.info => "info", 
			ExceptionArgument.key => "key", 
			ExceptionArgument.match => "match", 
			ExceptionArgument.obj => "obj", 
			ExceptionArgument.queue => "queue", 
			ExceptionArgument.stack => "stack", 
			ExceptionArgument.startIndex => "startIndex", 
			ExceptionArgument.value => "value", 
			ExceptionArgument.name => "name", 
			ExceptionArgument.mode => "mode", 
			ExceptionArgument.item => "item", 
			ExceptionArgument.options => "options", 
			ExceptionArgument.view => "view", 
			ExceptionArgument.sourceBytesToCopy => "sourceBytesToCopy", 
			_ => string.Empty, 
		};
	}

	internal static string GetResourceName(ExceptionResource resource)
	{
		string text = null;
		return resource switch
		{
			ExceptionResource.Argument_ImplementIComparable => "At least one object must implement IComparable.", 
			ExceptionResource.Argument_AddingDuplicate => "An item with the same key has already been added.", 
			ExceptionResource.ArgumentOutOfRange_BiggerThanCollection => "Larger than collection size.", 
			ExceptionResource.ArgumentOutOfRange_Count => "Count must be positive and count must refer to a location within the string/array/collection.", 
			ExceptionResource.ArgumentOutOfRange_Index => "Index was out of range. Must be non-negative and less than the size of the collection.", 
			ExceptionResource.ArgumentOutOfRange_InvalidThreshold => "The specified threshold for creating dictionary is out of range.", 
			ExceptionResource.ArgumentOutOfRange_ListInsert => "Index must be within the bounds of the List.", 
			ExceptionResource.ArgumentOutOfRange_NeedNonNegNum => "Non-negative number required.", 
			ExceptionResource.ArgumentOutOfRange_SmallCapacity => "capacity was less than the current size.", 
			ExceptionResource.Arg_ArrayPlusOffTooSmall => "Destination array is not long enough to copy all the items in the collection. Check array index and length.", 
			ExceptionResource.Arg_RankMultiDimNotSupported => "Only single dimensional arrays are supported for the requested action.", 
			ExceptionResource.Arg_NonZeroLowerBound => "The lower bound of target array must be zero.", 
			ExceptionResource.Argument_InvalidArrayType => "Target array type is not compatible with the type of items in the collection.", 
			ExceptionResource.Argument_InvalidOffLen => "Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection.", 
			ExceptionResource.Argument_ItemNotExist => "The specified item does not exist in this KeyedCollection.", 
			ExceptionResource.InvalidOperation_CannotRemoveFromStackOrQueue => "Removal is an invalid operation for Stack or Queue.", 
			ExceptionResource.InvalidOperation_EmptyQueue => "Queue empty.", 
			ExceptionResource.InvalidOperation_EnumOpCantHappen => "Enumeration has either not started or has already finished.", 
			ExceptionResource.InvalidOperation_EnumFailedVersion => "Collection was modified; enumeration operation may not execute.", 
			ExceptionResource.InvalidOperation_EmptyStack => "Stack empty.", 
			ExceptionResource.InvalidOperation_EnumNotStarted => "Enumeration has not started. Call MoveNext.", 
			ExceptionResource.InvalidOperation_EnumEnded => "Enumeration already finished.", 
			ExceptionResource.NotSupported_KeyCollectionSet => "Mutating a key collection derived from a dictionary is not allowed.", 
			ExceptionResource.NotSupported_ReadOnlyCollection => "Collection is read-only.", 
			ExceptionResource.NotSupported_ValueCollectionSet => "Mutating a value collection derived from a dictionary is not allowed.", 
			ExceptionResource.NotSupported_SortedListNestedWrite => "This operation is not supported on SortedList nested types because they require modifying the original SortedList.", 
			ExceptionResource.Serialization_InvalidOnDeser => "OnDeserialization method was called while the object was not being deserialized.", 
			ExceptionResource.Serialization_MissingKeys => "The Keys for this Hashtable are missing.", 
			ExceptionResource.Serialization_NullKey => "One of the serialized keys is null.", 
			ExceptionResource.Argument_InvalidType => "The type of arguments passed into generic comparer methods is invalid.", 
			ExceptionResource.Argument_InvalidArgumentForComparison => "Type of argument is not compatible with the generic comparer.", 
			ExceptionResource.InvalidOperation_NoValue => "Nullable object must have a value.", 
			ExceptionResource.InvalidOperation_RegRemoveSubKey => "Registry key has subkeys and recursive removes are not supported by this method.", 
			ExceptionResource.Arg_RegSubKeyAbsent => "Cannot delete a subkey tree because the subkey does not exist.", 
			ExceptionResource.Arg_RegSubKeyValueAbsent => "No value exists with that name.", 
			ExceptionResource.Arg_RegKeyDelHive => "Cannot delete a registry hive's subtree.", 
			ExceptionResource.Security_RegistryPermission => "Requested registry access is not allowed.", 
			ExceptionResource.Arg_RegSetStrArrNull => "RegistryKey.SetValue does not allow a String[] that contains a null String reference.", 
			ExceptionResource.Arg_RegSetMismatchedKind => "The type of the value object did not match the specified RegistryValueKind or the object could not be properly converted.", 
			ExceptionResource.UnauthorizedAccess_RegistryNoWrite => "Cannot write to the registry key.", 
			ExceptionResource.ObjectDisposed_RegKeyClosed => "Cannot access a closed registry key.", 
			ExceptionResource.Arg_RegKeyStrLenBug => "Registry key names should not be greater than 255 characters.", 
			ExceptionResource.Argument_InvalidRegistryKeyPermissionCheck => "The specified RegistryKeyPermissionCheck value is invalid.", 
			ExceptionResource.NotSupported_InComparableType => "A type must implement IComparable<T> or IComparable to support comparison.", 
			ExceptionResource.Argument_InvalidRegistryOptionsCheck => "The specified RegistryOptions value is invalid.", 
			ExceptionResource.Argument_InvalidRegistryViewCheck => "The specified RegistryView value is invalid.", 
			_ => string.Empty, 
		};
	}
}
