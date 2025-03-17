// ------------------------------------------------------------------
// Copyright (c) Magnopus LLC. All Rights Reserved.
// ------------------------------------------------------------------

using System;

namespace Csp
{
    public class ProgressEventArgs : EventArgs
    {
        public float Progress;

        public ProgressEventArgs(float progress)
        {
            Progress = progress;
        }
    }
}
