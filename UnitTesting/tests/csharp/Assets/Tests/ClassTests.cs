/*
 * Copyright 2025 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using NUnit.Framework;
using System;

namespace Csp.Tests
{
    public class ClassTests
    {
        [Test]
        public void CreateAndDestroySimpleClass()
        {
            // Calling the constructor creates an instance of the native class.
            var simpleClass = new SimpleClass();
            Assert.IsTrue(simpleClass.PointerIsValid);

            // Verify the internal pointer is set.
            // Note that we have to use reflection here to access the internal field.
            var ptrField = typeof(NativeClassWrapper).GetField("_ptrValue", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            IntPtr ptrValue = (IntPtr)ptrField.GetValue(simpleClass);
            Assert.IsNotNull(ptrValue);

            // Verify that the instance owns the native pointer.
            // Note that we have to use reflection here to access the internal field.
            var ownsPtrField = typeof(NativeClassWrapper).GetField("_ownsPtr", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            var ownsPtrValue = ownsPtrField.GetValue(simpleClass);
            Assert.IsTrue((bool)ownsPtrValue);

            // Calling Dispose will call the destructor on the native class and
            // mark this instance as disposed.
            simpleClass.Dispose();

            // Once Dispose is called, the internal pointer is no longer valid.
            Assert.IsFalse(simpleClass.PointerIsValid);

            // Verify the internal pointer has been nulled.
            IntPtr ptrValueDisposed = (IntPtr)ptrField.GetValue(simpleClass);
            Assert.AreEqual(IntPtr.Zero, ptrValueDisposed);
        }

        [Test]
        public void GetValueReturnsCorrectValue()
        {
            var simpleClass = new SimpleClass();
            int value = simpleClass.GetValue();
            Assert.AreEqual(42, value);
        }

        [Test]
        public void MultipleDisposes()
        {
            var simpleClass = new SimpleClass();

            // Call Dispose multiple times and ensure no errors occur.
            simpleClass.Dispose();
            simpleClass.Dispose();
        }

        [Test]
        public void AccessAfterDisposeThrows()
        {
            var simpleClass = new SimpleClass();
            simpleClass.Dispose();

            // Accessing the pointer after Dispose should throw a NullReferenceException rather
            // than creating a segfault.
            var ex = Assert.Throws<NullReferenceException>(() => { int value = simpleClass.GetValue(); });
            StringAssert.Contains("Attempting to access a null pointer", ex.Message);
        }

        [Test]
        public void TestDerivedType()
        {
            // Ensure that a derived class can be created and destroyed without issues.
            using (var derivedClass = new DerivedClass())
            {
                Assert.IsTrue(derivedClass.PointerIsValid);
            }
        }
    }
}