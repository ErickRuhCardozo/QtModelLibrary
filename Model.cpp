#include <QSqlQuery>
#include <QSqlError>
#include "Model.hpp"

Model::Model(QObject* parent)
    : QObject{parent}
{
}

model_id_t Model::id() const
{
    return m_id;
}

bool Model::isSaved() const
{
    return m_id != 0;
}

bool Model::isModified() const
{
    return !m_modifiedProperties.isEmpty();
}

bool Model::insert()
{
    if (isSaved())
        return false;

    return execDML(insertQuery());
}

bool Model::update()
{
    if (!isSaved()) {
        qWarning() << "Model is not saved";
        return false;
    }

    if (!isModified())
        return false;

    return execDML(updateQuery());
}

bool Model::deleteFromDatabase()
{
    if (!execDML(deleteQuery()))
        return false;

    deleteLater();
    return true;
}

bool Model::load(model_id_t id, bool eagerLoad)
{
    QSqlQuery query;
    QStringList queryStr;
    queryStr << "SELECT ";

    forEachProperty([&queryStr](auto metaProperty){
        queryStr << metaProperty.name() << ",";
    });

    queryStr.removeLast(); // trailing comma
    queryStr << " FROM " << tableName() << " WHERE id = :id";

    if (!query.prepare(queryStr.join(""))) {
        qCritical() << "Could not prepare SELECT query:" << query.lastError().text();
        return false;
    }

    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Could not execute SELECT query:" << query.lastError().text();
        return false;
    }

    if (!query.first())
        return false;

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        QMetaProperty metaProperty = metaObject()->property(i);
        QVariant dbValue = query.value(metaProperty.name());

        if (isPropertyModel(metaProperty) && dbValue.typeId() == QMetaType::LongLong) {
            if (eagerLoad) {
                Model* related = createRelatedInstance(metaProperty);

                if (related->load(dbValue.toUInt(), eagerLoad)) {
                    dbValue = QVariant::fromValue(related);
                }
            } else {
                setProperty(QString("%1Id").arg(metaProperty.name()).toLocal8Bit(), dbValue);
            }
        }

        if (!metaProperty.write(this, dbValue) && eagerLoad) {
            qWarning() << QString(R"(Could not set property "%1")").arg(metaProperty.name());
            return false;
        }
    }

    return true;
}

bool Model::loadRelated(const QString& propertyName, bool eagerLoad)
{
    QString idPropName = QString("%1Id").arg(propertyName);

    if (!dynamicPropertyNames().contains(idPropName.toUtf8()))
        return false;

    model_id_t relatedId = property(idPropName.toLocal8Bit()).toUInt();
    int relatedPropertyIndex = metaObject()->indexOfProperty(propertyName.toLocal8Bit());
    QMetaProperty relatedMetaProperty = metaObject()->property(relatedPropertyIndex);
    Model* related = createRelatedInstance(relatedMetaProperty);

    if (!related->load(relatedId, eagerLoad))
        return false;

    relatedMetaProperty.write(this, QVariant::fromValue(related));
    return true;
}

bool Model::isPropertyModel(const QMetaProperty& metaProperty)
{
    auto metaObject = metaProperty.metaType().metaObject();
    return metaObject != nullptr && metaObject->inherits(&staticMetaObject);
}

void Model::setId(model_id_t id)
{
    if (id == m_id)
        return;

    m_id = id;
    emit idChanged();
}

void Model::setModified(const QString& propertyName)
{
    m_modifiedProperties.insert(propertyName);
}

const QSet<const QString>& Model::modifiedProperties() const
{
    return m_modifiedProperties;
}

QVariant Model::insertQuery() const
{
    QStringList queryStr;
    queryStr << "INSERT INTO " << tableName() << " (";

    forEachProperty([&queryStr](auto metaProperty) {
        queryStr << metaProperty.name() << ",";
    });

    queryStr.removeLast(); // trailing comma
    queryStr << ") VALUES (";

    forEachProperty([&queryStr](auto metaProperty) {
        queryStr << QString(":%1").arg(metaProperty.name()) << ",";
    });

    queryStr.removeLast();
    queryStr << ")";
    QSqlQuery query;

    if (!query.prepare(queryStr.join(""))) {
        qCritical() << "Could not prepare INSERT query:" << query.lastError().text();
        return QVariant();
    }

    forEachProperty([&query, this](auto metaProperty) {
        query.bindValue(QString(":%1").arg(metaProperty.name()), metaProperty.read(this));
    });

    return QVariant::fromValue(query);
}

QVariant Model::updateQuery() const
{
    QStringList queryStr;
    queryStr << "UPDATE " << tableName() << " SET ";

    for (const QString& propertyName : modifiedProperties())
        queryStr << propertyName << QString(" = :%1,").arg(propertyName); // Don't put extra space at the end

    queryStr.last().chop(1); // remove trailing comma
    queryStr << " WHERE id = :id";
    QSqlQuery query;

    if (!query.prepare(queryStr.join(""))) {
        qCritical() << "Could not prepare UPDATE query:" << query.lastError().text();
        return QVariant();
    }

    for (const QString& propertyName : modifiedProperties()) {
        int propertyIndex = metaObject()->indexOfProperty(propertyName.toLocal8Bit());
        QMetaProperty property = metaObject()->property(propertyIndex);
        QVariant value = property.read(this);
        QString paramName = QString(":%1").arg(propertyName);

        if (value.canConvert<Model*>()) {
            Model* model = value.value<Model*>();

            if (!model->isSaved() && !model->insert())
                return QVariant();
            else if (model->isModified() && !model->update())
                return QVariant();

            query.bindValue(paramName, model->id());
        } else {
            query.bindValue(paramName, value);
        }
    }

    query.bindValue(":id", id());
    return QVariant::fromValue(query);
}

QVariant Model::deleteQuery() const
{
    QString queryStr = QString("DELETE FROM %1 WHERE id = ?").arg(tableName());
    QSqlQuery query;

    if (!query.prepare(queryStr)) {
        qCritical() << "Couldn't prepare DELETE query:" << query.lastError().text();
        return QVariant();
    }

    query.addBindValue(m_id);
    return QVariant::fromValue(query);
}

bool Model::execDML(const QVariant& variant)
{
    if (!variant.isValid() || !variant.canConvert<QSqlQuery>())
        return false;

    QSqlQuery query = variant.value<QSqlQuery>();

    if (!query.exec()) {
        qCritical() << "Unable to execute query:" << query.lastError().text();
        return false;
    }

    if (query.lastInsertId().isValid()) {
        setId(query.lastInsertId().toUInt());
    }

    return true;
}

Model* Model::createRelatedInstance(const QMetaProperty& relatedProperty) const
{
    QMetaType relatedMetaType = relatedProperty.metaType();
    const QMetaObject* relatedMetaObject = relatedMetaType.metaObject();
    return (Model*)relatedMetaObject->newInstance();
}

void Model::forEachProperty(std::function<void (const QMetaProperty&)> action) const
{
    int start = metaObject()->propertyOffset();
    int end = metaObject()->propertyCount();

    for (int i = start; i < end; ++i) {
        QMetaProperty metaProperty = metaObject()->property(i);
        action(metaProperty);
    }
}
