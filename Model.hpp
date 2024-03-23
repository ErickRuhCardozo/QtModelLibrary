#pragma once

#include <QSet>
#include <QObject>
#include <functional>
#include <QMetaProperty>
#include "QtModelLibrary_global.hpp"

using model_id_t = quint64;

/**
 * @brief The base class for all models that can be saved to a DBMS.
 */
class QTMODELLIBRARY_EXPORT Model : public QObject
{
    Q_OBJECT
    Q_PROPERTY(model_id_t id READ id NOTIFY idChanged FINAL)

public:
    explicit Model(QObject* parent = nullptr);

    /**
     * @brief The DBMS id of this model.
     * @return The Model id.
     */
    model_id_t id() const;

    /**
     * @brief Checks whether or not this Model is saved in the database.
     * @return true if it's saved, false otherwise.
     */
    bool isSaved() const;

    /**
     * @brief Checks whether this Model had any of its properties changed
     *        since it was instantiated or loaded from the database.
     * @return true if it has modified properties, false if not.
     */
    bool isModified() const;

    /**
     * @brief Attempts to insert the Model in the database. If the attempt
     *        succeeds, the id of the Model is atomatiacally update to
     *        match the DBMS id.
     * @return true if the Model was successfully inserted, false otherwise.
     */
    bool insert();

    /**
     * @brief Attempts to update the Model in the database. If the model
     *        is not yet saved or doesn't have any modified properties,
     *        this method returns false without executing any query in
     *        the database.
     * @return true if the Model was successfully updated in the database,
     *         false otherwise.
     */
    bool update();

    /**
     * @brief Attepts to delete the Model from the database. If the Model
     *        could be deleted, this method calls deleteLater on the instance,
     *        hence, the object should not be used. This method doesn't delete
     *        related Models.
     * @return true if the model was successfully deleted from
     *         the database, false otherwise.
     */
    bool deleteFromDatabase(); // Wish I could only use "delete" :P

    /**
     * @brief Attempts to load the Model from the database with the given id.
     *        By default, this method attempts to eagerly load related Models.
     *        If the related Models could not be load this method returns false.
     *        If the consumer specifies to not eager load related Models, when
     *        this method encounters a Model property on this instance with a
     *        matching id loaded from the database, it adds the id as a dynamic
     *        property that can later be used to Lazy Load the related Model.
     * @param id The database id of the Model.
     * @param eagerLoad Should this method load related Models or not.
     * @return true if the Model and its related Models (if eager loading) could
     *         be loaded from the database, false otherwise.
     */
    virtual bool load(model_id_t id, bool eagerLoad = true);

    /**
     * @brief Attempts to load a related Model that was not eager loaded (Lazy Loading).
     * @param propertyName The name of the related property to load from the database.
     * @param eagerLoad Should this method attempt to eager load related Models.
     * @return true if the related Model and its related Model properties could be loaded
     *         from the database, false otherwise.
     */
    virtual bool loadRelated(const QString& propertyName, bool eagerLoad = true);

    /**
     * @brief Checks wheter a property is of type Model.
     * @param metaProperty The meta-property of the property to test.
     * @return true if the type of the property is a subclass of Model,
     *         false otherwise.
     */
    static bool isPropertyModel(const QMetaProperty& metaProperty);

signals:
    void idChanged();

protected:
    void setId(model_id_t id);

    /**
     * @brief This method must be called by derived classes in their setter methods.
     *        It stores the name of the modified property which is later use to build
     *        the Update query.
     * @param propertyName The name of the Q_PROPERTY that is being modified.
     */
    void setModified(const QString& propertyName);

    /**
     * @brief This method is provided as a helper to subclasses that override the
     *        updateQuery method.
     * @return A set with the names of the properties that were modified since this
     *         model was created or loaded from the database.
     */
    const QSet<const QString>& modifiedProperties() const;

    virtual QString tableName() const = 0;

    /**
     * @brief Attempts to prepare an insert query. If a QSqlQuery couldn't be prepared
     *        this method returns an invalid QVariant.
     * @return A QSqlQuery if one could be prepared or an invalid QVariant otherwise.
     */
    virtual QVariant insertQuery() const;

    /**
     * @brief Attempts to prepare an update query. If a QSqlQuery couldn't be prepared
     *        this method returns an invalid QVariant.
     * @return A QSqlQuery if one could be prepared or an invalid QVariant otherwise.
     */
    virtual QVariant updateQuery() const;

    /**
     * @brief Attempts to prepare an delete query. If a QSqlQuery couldn't be prepared
     *        this method returns an invalid QVariant.
     * @return A QSqlQuery if one could be prepared or an invalid QVariant otherwise.
     */
    virtual QVariant deleteQuery() const;


private:
    model_id_t m_id;
    QSet<const QString> m_modifiedProperties;

    /**
     * @brief Attempts to create an instance of a subclass of Model for a related property.
     * @param relatedProperty The meta-property of the related Model property.
     * @return An instance of a subclass of Model upon success or nullptr on failure.
     */
    Model* createRelatedInstance(const QMetaProperty& relatedProperty) const;

    bool execDML(const QVariant& variant);
    void forEachProperty(std::function<void (const QMetaProperty&)> action) const;
};
