# Runtime QML for Qt

**Written by**: *Benjamin Balga.*
**Copyright**: ***2018***, *Benjamin Balga*, released under BSD license.


## About

This is a base project to get runtime QML reload in your Qt project.
It allows you to reload all QML code at runtime, without recompiling or restarting your app, saving time.

With auto-reload, QML files are watched (based on the QRC file) and reloaded when you save them, or can trigger it manually.

On reload, all windows are closed, and the main window is reloaded. All "QML-only data" is lost, so use links to C++ models/properties as needed.

It only works with Window component as top object, or QQuickWindow subclasses.

### Examples
Example project is located here: https://github.com/GIPdA/runtimeqml_examples


## How to use it in your project

Clone the repo into your project (or copy-paste the ```runtimeqml``` folder) and import the ```.pri``` project file into your ```.pro``` file:

	include(runtimeqml/runtimeqml.pri)


### With Qbs
The Qbs project file includes RuntimeQML as a static library. Check the example project to see how to include it in your project.


## Usage

Include ```runtimeqml.h``` header file, and create the RuntimeQML object (after the QML engine) :

	RuntimeQML *rt = new RuntimeQML(&engine, QRC_SOURCE_PATH"/qml.qrc");

The second argument is the path to your qrc file listing all your QML files, needed for the auto-reload feature only. You can omit it if you don't want auto-reload.
```QRC_SOURCE_PATH``` is defined in the ```.pri/.qbs``` file to its parent path, just to not have to manually set an absolute path...


Set the "options" you want, or not:

	rt->noDebug(); // Removes debug prints
	rt->setAutoReload(true); // Enable auto-reload (begin to watch files)
	//rt->setCloseAllOnReload(false); // Don't close all windows on reload. Not advised!
	rt->setMainQmlFilename("main.qml"); // This is the file that loaded on reload, default is "main.qml"

For the auto-reload feature:

	rt->addSuffix("conf"); // Adds a file suffix to the "white list" for watched files. "qml" is already in.
    rt->ignorePrefix("/test"); // Ignore a prefix in the QRC file.
    rt->ignoreFile("/Page2.qml"); // Ignore a file name with prefix (supports classic wildcard matching)


Then load the main QML file :

	rt->reload();
    
And you're all set!


You can also check the test project. Beware, includes and defines differs a bit...


## Manual reload

Add the RuntimeQML object to the QML context:

	engine.rootContext()->setContextProperty("RuntimeQML", rt);
	
Trigger the reload when and where you want, like with a button:

	Button {
        text: "Reload"
        onClicked: {
            RuntimeQML.reload();
        }
    }

You can do it in C++ too, of course.



## License
See LICENSE file.
