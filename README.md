# Motivation
Have you ever cought yourself repeating SQL-related code in the first hours of a new project? Well, me too. Thinking on that, I came up with a simple class that aims to avoid or, at least, reduce the amount of SQL-related code you need to write to persist and retrieve objects from a database.
This still in its early days. The code have not been thoroughly tested and is not considered production ready.

# How?
This projects make extensive use of the [Qt Meta-Object System](https://doc.qt.io/qt-6/metaobjects.html) to persist the values of QObject's properties in an DBMS as well as retrieving them and automatically assign them to their respective properties.

# Features
Currently, there are not much exciting features other than the bare minimum:
* Easy persistion/retrieval objects to/from databases
* Eager Loding and Lazy Loading of related entities
* That's it :)

# Performance
Since this projects uses reflection/introspection and prepared queries (aiming for security), the overhead is quite considerable. Though, the dev-time gaining and reduction of SQL-related code may pay it off...

# How To
To begin with, just create a new class that inherits the Model class:
```cpp
class Person : public Model
{
    Q_OBJECT
    Q_PROPERTY(QString fullName READ fullName WRITE setFullName NOTIFY fullNameChanged FINAL)
    Q_PROPERTY(QDate birth READ birth WRITE setBirth NOTIFY birthChanged FINAL)
    Q_PROPERTY(Address* address READ address WRITE setAddress NOTIFY addressChanged FINAL)

public:
    Q_INVOKABLE explicit Person(QObject* parent = nullptr) : Model{parent} { }
    // Getters and Setters ommited for brevity
protected:
    inline QString tableName() const override { return "people"; }
};
```
Pay attention the the constructor: The derived class must provide a parameterless constructor marked as [Q_INVOKABLE](https://doc.qt.io/qt-6/qobject.html#Q_INVOKABLE).
This is needed in order to provide related Model loading.
The derived class must also implement the tableName() method to provide the DBMS table name. This is used by the Model default implementation to build the SQL queries.
All the persisted properties must be declared as [Q_PROPERTY](https://doc.qt.io/qt-6/properties.html).

# Persisting Objects
The base Model class provides an `insert` method that can be used to persist objects in the database:
```cpp
#include <QCoreApplication>
#include "Person.hpp"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    Person me(&app);
    me.setFullName("Erick Ruh Cardozo");
    me.setBirth(QDate(1998, 10, 28));

    if (me.insert())
        qInfo() << "I'm saved B-)";
    else
        qCritical() << "Help Jesus :(";

    return app.exec();
}
```
The `insert` method returns whether the Model could or not be saved in the database.

# Loading Objects
To load objects from the database, create an instance of the desired Model and call `load` on it passing the DBMS id:
```cpp
Person me(&app);

if (me.load(1))
  qInfo() << me.fullName() << "is" << QDate::currentDate().year() - me.birth().year() << "years old";
else
  qCritical() << "No person found";
```

# Lazy Loading
By default, the Model implementation eager loads any related Model property. You can pass a second parameter to the `insert` method to opt-out eager loading:
```cpp
Person me(&app);
me.load(1, false);
```
When you specify to not eager load the related Model properties, when an Model property is encountered when loading a Model from the database, the id of the related Model is added as a [dynamic property](https://doc.qt.io/qt-6/properties.html#dynamic-properties)
that is used later to Lazy Load the related Model. When you need to load a related Model property from the database, just call the `loadRelated` method specifying the property name of the related Model to load:
```cpp
Person me(&app);
me.load(1, false); // Opt-out from eager load

if (me.loadRelated("address"))
    qInfo() << me.fullName() << "lives at" me.address()->toString();
else
    qCritical() << "The address could not be loaded";
```
By default, the loadRelated method tries to eager load related Model properties when they are found in the loading property. You can opt-out from this behaviour by specifing a second parameter to the `loadRelated` function:
```cpp
Person me(&app);
me.load(1, false);
// Suppose the Address Model has a related City Model property that we won't need now.
// We pass false to the 2'nd parameter of loadRelated. This way, when a related Model property
// is found it will not be loaded eagerly
me.loadRelated("address", false);
qInfo() << me.address()->city()->name(); // SEGFAULT
```

I hope this simple project helps as many people as possible. If you want to help, please send a PR ;)
