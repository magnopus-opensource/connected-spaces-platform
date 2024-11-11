using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace Csp
{
    public static class WrapperHelper
    {
        internal static string NativeUTF8ToString(IntPtr buf, int len = -1)
        {
            unsafe
            {
                if (buf == IntPtr.Zero)
                    return null;

                var _buf = (byte*)buf;

                if (len == -1)
                {
                    len = 0;

                    while (_buf[len] != 0)
                        len++;
                }

                return Encoding.UTF8.GetString(_buf, len);
            }
        }

        internal static IntPtr StringToNativeUTF8(string value)
        {
            var bytes = Encoding.UTF8.GetBytes(value);
            var length = bytes.Length;
            var buf = Marshal.AllocHGlobal(length + 1);
            Marshal.Copy(bytes, 0, buf, length);

            unsafe
            {
                var _buf = (byte*)buf;
                _buf[length] = 0;
            }

            return buf;
        }

        internal static IntPtr ObjectToIntPtr<T>(T value, bool valueIsClass)
        {
            if (valueIsClass) return (value as NativeClassWrapper)._ptr;
            if (value is string s) return StringToNativeUTF8(s);
            if (value is ushort u16) return (IntPtr)u16;
            if (value is short i16) return (IntPtr)i16;
            if (value is int i32) return (IntPtr)i32;
            if (value is uint u32) return (IntPtr)u32;
            if (value is long i64) return (IntPtr)i64;
            if (value is ulong u64) return (IntPtr)u64;
            if (value.GetType().IsEnum) return (IntPtr)Convert.ToInt32(value);

            return IntPtr.Zero;
        }

        internal static object IntPtrToObject<T>(IntPtr value, bool valueIsClass)
        {
            var t = typeof(T);

            if (valueIsClass) return (T)Activator.CreateInstance(t, BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { new NativePointer { Pointer = value } }, null);
            if (t == typeof(string)) return NativeUTF8ToString(value);
            if (t == typeof(ushort)) return (ushort)value;
            if (t == typeof(short)) return (short)value;
            if (t == typeof(int)) return (int)value;
            if (t == typeof(uint)) return (uint)value;
            if (t == typeof(long)) return (long)value;
            if (t == typeof(ulong)) return (ulong)value;
            if (t.IsEnum) return Enum.ToObject(t, (int)value);

            return default;
        }

        internal static object NativePointerToObject<T>(NativePointer value, bool valueIsClass)
        {
            var t = typeof(T);

            if (valueIsClass) return (T)Activator.CreateInstance(t, BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { value }, null);
            if (t == typeof(string)) return NativeUTF8ToString(value.Pointer);
            if (t == typeof(ushort)) return (ushort)value.Pointer;
            if (t == typeof(short)) return (short)value.Pointer;
            if (t == typeof(int)) return (int)value.Pointer;
            if (t == typeof(uint)) return (uint)value.Pointer;
            if (t == typeof(long)) return (long)value.Pointer;
            if (t == typeof(ulong)) return (ulong)value.Pointer;
            if (t.IsEnum) return Enum.ToObject(t, (int)value.Pointer);

            return default;
        }

        [Conditional("DEBUG")]
        internal static void CheckNativePointer(NativePointer value)
        {
            if (value.Equals(NativePointer.Zero))
                throw new NullReferenceException();
        }
    }
}
