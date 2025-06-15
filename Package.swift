// swift-tools-version: 6.0

import PackageDescription

func resources(shaders: [String], data: [String] = []) -> [PackageDescription.Resource] {
	shaders.flatMap { name in
#if os(macOS)
		[ .process("../../../data/shaders/\(name).metallib") ]
#else
		[
			.process("../../../data/shaders/\(name).vtx.spv"),
			.process("../../../data/shaders/\(name).frg.spv"),
		]
#endif
	} + data.map { resource in .process("../../../data/\(resource)") }
}

let package = Package(
	name: "NeHe-SDL_GPU",
	products: [
		.executable(name: "Lesson1", targets: [ "Lesson1" ]),
		.executable(name: "Lesson2", targets: [ "Lesson2" ]),
		.executable(name: "Lesson3", targets: [ "Lesson3" ]),
		.executable(name: "Lesson4", targets: [ "Lesson4" ]),
		.executable(name: "Lesson5", targets: [ "Lesson5" ]),
		.executable(name: "Lesson6", targets: [ "Lesson6" ]),
		.executable(name: "Lesson7", targets: [ "Lesson7" ]),
		.executable(name: "Lesson8", targets: [ "Lesson8" ]),
		.executable(name: "Lesson9", targets: [ "Lesson9" ]),
		.executable(name: "Lesson10", targets: [ "Lesson10" ]),
	],
	dependencies: [
		.package(url: "https://github.com/GayPizzaSpecifications/SDL3Swift.git", branch: "main"),
		.package(url: "https://github.com/keyvariable/kvSIMD.swift.git", .upToNextMajor(from: "1.1.0")),
	],
	targets: [
		.target(
			name: "NeHe",
			dependencies: [
				.product(name: "SDLSwift", package: "SDL3Swift"),
				.product(name: "kvSIMD", package: "kvSIMD.swift"),
			],
			path: "src/swift/NeHe"),
		.executableTarget(name: "Lesson1", dependencies: [ "NeHe" ], path: "src/swift/Lesson1"),
		.executableTarget(name: "Lesson2", dependencies: [ "NeHe" ], path: "src/swift/Lesson2", resources:
			resources(shaders: [ "lesson2" ])),
		.executableTarget(name: "Lesson3", dependencies: [ "NeHe" ], path: "src/swift/Lesson3", resources:
			resources(shaders: [ "lesson3" ])),
		.executableTarget(name: "Lesson4", dependencies: [ "NeHe" ], path: "src/swift/Lesson4", resources:
			resources(shaders: [ "lesson3" ])),
		.executableTarget(name: "Lesson5", dependencies: [ "NeHe" ], path: "src/swift/Lesson5", resources:
			resources(shaders: [ "lesson3" ])),
		.executableTarget(name: "Lesson6", dependencies: [ "NeHe" ], path: "src/swift/Lesson6", resources:
			resources(shaders: [ "lesson6" ], data: [ "NeHe.bmp" ])),
		.executableTarget(name: "Lesson7", dependencies: [ "NeHe" ], path: "src/swift/Lesson7", resources:
			resources(shaders: [ "lesson6", "lesson7" ], data: [ "Crate.bmp" ])),
		.executableTarget(name: "Lesson8", dependencies: [ "NeHe" ], path: "src/swift/Lesson8", resources:
			resources(shaders: [ "lesson7", "lesson8" ], data: [ "Glass.bmp" ])),
		.executableTarget(name: "Lesson9", dependencies: [ "NeHe" ], path: "src/swift/Lesson9", resources:
			resources(shaders: [ "lesson9" ], data: [ "Star.bmp" ])),
		.executableTarget(name: "Lesson10", dependencies: [ "NeHe" ], path: "src/swift/Lesson10", resources:
			resources(shaders: [ "lesson6" ], data: [ "Mud.bmp", "World.txt" ])),
	]
)
