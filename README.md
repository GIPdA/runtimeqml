# Runtime QML for Qt 6

**Written by**: *Benjamin Balga.*
**Copyright**: ***2022***, *Benjamin Balga*, released under BSD license.


**Updated version**: This new version make use of an URL Interceptor to replace QRC files by those on the filesystem. This allows to load multiple QRC files, and even works on libraries! Supported in Qt 6, the feature is undocumented in Qt5 but should work.


## About

RuntimeQml allows to hot-reload QML files at runtime.

With auto-reload, QML files are watched (based on the QRC file) and reloaded when you save them, or can trigger it manually.

On reload, all windows are closed, and the main window is reloaded. All "QML-only data" is lost, so use links to C++ models/properties as needed.

It only works with Window component as top object, or QQuickWindow subclasses.

### Examples
Example project is located here: https://github.com/GIPdA/runtimeqml_examples


## How to use it in your project

Clone the repo into your project (or copy-paste the ```runtimeqml``` folder) and import the ```.pri``` project file into your ```.pro``` file:

	include(runtimeqml/runtimeqml.pri)


### With Qbs
Add a dependency to the RuntimeQml project. Check the example project to see how to include it in your project.


## Usage

Include ```runtimeqml.hpp``` header file, and create the RuntimeQml object (after the QML engine) :

	RuntimeQml *rt = new RuntimeQML(&engine);
	rt->parseQrc(QRC_SOURCE_PATH "/qml.qrc");

The second line parse the QRC file listing all your QML files, needed to link QRC paths with the filesystem. Multiple QRC files can be parsed.

```QRC_SOURCE_PATH``` is defined in the ```.pri/.qbs``` file to its parent path, just to not have to manually set an absolute path...


Set the "options" you want, or not:

	rt->setAutoReload(true); // Enable auto-reload

For the auto-reload feature:

	rt->addFileSuffixFilter("conf"); // Adds a file suffix to the "white list" for watched files. "qml" is already in.
	rt->addQrcPrefixIgnoreFilter("noreload"); // Ignore a prefix from auto-reload.
    rt->addIgnoreFilter("*NoReload.qml"); // Ignore some files from auto-reload. Supports glob wildcards.


Then load the main QML file as you would do with QQmlApplicationEngine :

	rt->load(QStringLiteral("qrc:/qml/main.qml")); // Use reload() for subsequent reloads.
    
And you're all set!


You can also check the test project. Beware, includes and defines differs a bit...


## Manual reload

Add the RuntimeQML object to the QML context:

	engine.rootContext()->setContextProperty("RuntimeQml", rt);
	
Trigger the reload when and where you want, like with a button:

	Button {
        text: "Reload"
        onClicked: {
            RuntimeQml.reload();
        }
    }

You can do it in C++ too, of course.



## License
See LICENSE file.
