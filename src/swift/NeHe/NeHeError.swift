/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation

public enum NeHeError: Error
{
	case fatalError(StaticString)
	case sdlError(StaticString, String)
}

extension NeHeError: LocalizedError
{
	public var errorDescription: String?
	{
		switch self
		{
		case .fatalError(let why): "\(why)"
		case .sdlError(let fname, let message): "\(fname): \(message)"
		}
	}
}
