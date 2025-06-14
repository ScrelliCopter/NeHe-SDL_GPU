// swift-tools-version: 6.0

import PackageDescription

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
	],
	targets: [
		.target(
			name: "NeHe",
			dependencies: [ .product(name: "SDLSwift", package: "SDL3Swift") ],
			path: "src/swift/NeHe"),
		.executableTarget(name: "Lesson1", dependencies: [ "NeHe" ], path: "src/swift/Lesson1"),
		.executableTarget(name: "Lesson2", dependencies: [ "NeHe" ], path: "src/swift/Lesson2", resources: [
			.process("../../../data/shaders/lesson2.metallib") ]),
		.executableTarget(name: "Lesson3", dependencies: [ "NeHe" ], path: "src/swift/Lesson3", resources: [
			.process("../../../data/shaders/lesson3.metallib") ]),
		.executableTarget(name: "Lesson4", dependencies: [ "NeHe" ], path: "src/swift/Lesson4", resources: [
			.process("../../../data/shaders/lesson3.metallib") ]),
		.executableTarget(name: "Lesson5", dependencies: [ "NeHe" ], path: "src/swift/Lesson5", resources: [
			.process("../../../data/shaders/lesson3.metallib") ]),
		.executableTarget(name: "Lesson6", dependencies: [ "NeHe" ], path: "src/swift/Lesson6", resources: [
			.process("../../../data/shaders/lesson6.metallib"),
			.process("../../../data/NeHe.bmp") ]),
		.executableTarget(name: "Lesson7", dependencies: [ "NeHe" ], path: "src/swift/Lesson7", resources: [
			.process("../../../data/shaders/lesson6.metallib"),
			.process("../../../data/shaders/lesson7.metallib"),
			.process("../../../data/Crate.bmp") ]),
		.executableTarget(name: "Lesson8", dependencies: [ "NeHe" ], path: "src/swift/Lesson8", resources: [
			.process("../../../data/shaders/lesson6.metallib"),
			.process("../../../data/shaders/lesson7.metallib"),
			.process("../../../data/Glass.bmp") ]),
		.executableTarget(name: "Lesson9", dependencies: [ "NeHe" ], path: "src/swift/Lesson9", resources: [
			.process("../../../data/shaders/lesson9.metallib"),
			.process("../../../data/Star.bmp") ]),
		.executableTarget(name: "Lesson10", dependencies: [ "NeHe" ], path: "src/swift/Lesson10", resources: [
			.process("../../../data/shaders/lesson6.metallib"),
			.process("../../../data/Mud.bmp"),
			.process("../../../data/World.txt") ]),
	],
)
