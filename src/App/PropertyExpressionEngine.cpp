#include "PreCompiled.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include "Expression.h"
#include "PropertyExpressionEngine.h"
#include "PropertyStandard.h"
#include "PropertyUnits.h"
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

using namespace App;
using namespace boost;

typedef adjacency_list< listS, vecS, directedS > DiGraph;
typedef std::pair<int, int> Edge;

TYPESYSTEM_SOURCE(App::PropertyExpressionEngine , App::Property);

PropertyExpressionEngine::PropertyExpressionEngine(DocumentObject * _docObj)
    : Property()
    , docObj(_docObj)
    , running(false)
{
}

PropertyExpressionEngine::~PropertyExpressionEngine()
{
    std::map<const Path, const Expression*>::iterator i = expressions.begin();

    while (i != expressions.end()) {
        delete i->second;
        ++i;
    }
}

unsigned int PropertyExpressionEngine::getMemSize() const
{
    return 0;
}

Property *PropertyExpressionEngine::Copy() const
{
    PropertyExpressionEngine * engine = new PropertyExpressionEngine(docObj);

    for (std::map<const Path, const Expression*>::const_iterator it = expressions.begin(); it != expressions.end(); ++it) {
        engine->expressions[it->first] = it->second->copy();
    }
    engine->evaluationOrder = evaluationOrder;

    return engine;
}

void PropertyExpressionEngine::Paste(const Property &from)
{
    const PropertyExpressionEngine * fromee = static_cast<const PropertyExpressionEngine*>(&from);

    aboutToSetValue();
    docObj = fromee->docObj;
    std::map<const Path, const Expression*>::iterator i = expressions.begin();
    while (i != expressions.end()) {
        delete i->second;
        ++i;
    }
    expressions.clear();

    for (std::map<const Path, const Expression*>::const_iterator it = fromee->expressions.begin(); it != fromee->expressions.end(); ++it) {
        expressions[it->first] = it->second->copy();
    }

    evaluationOrder = fromee->evaluationOrder;

    hasSetValue();
}

void PropertyExpressionEngine::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<ExpressionEngine count=\"" <<  expressions.size() <<"\">" << std::endl;
    writer.incInd();
    for (std::map<const Path, const Expression*>::const_iterator it = expressions.begin(); it != expressions.end(); ++it) {
        writer.Stream() << writer.ind() << "<Expression path=\"" <<  it->first.toString() <<"\" " <<
                           "expression=\"" << it->second->toString() << "\"" <<
                           "/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</ExpressionEngine>" << std::endl;
}

void PropertyExpressionEngine::Restore(Base::XMLReader &reader)
{
    reader.readElement("ExpressionEngine");

    int count = reader.getAttributeAsFloat("count");

    for (int i = 0; i < count; ++i) {
        reader.readElement("Expression");
        Path path = Path::parse(reader.getAttribute("path"));
        Expression * expression = ExpressionParser::parse(docObj, reader.getAttribute("expression"));

        setValue(path, expression);
    }

    reader.readEndElement("ExpressionEngine");
}

struct cycle_detector : public boost::dfs_visitor<> {
    cycle_detector( bool& has_cycle)
      : _has_cycle(has_cycle) { }

    template <class Edge, class Graph>
    void back_edge(Edge, Graph&) {
      _has_cycle = true;
    }
  protected:
    bool& _has_cycle;
};

static void buildGraphStructures(const Path & path,
                 const Expression * expression,
                 std::map<Path, int> & nodes,
                 std::map<int, Path> & revNodes,
                 std::vector<Edge> & edges)
{
    std::set<Path> deps;

    /* Insert target property into nodes structure */
    if (nodes.find(path) == nodes.end()) {
        int s = nodes.size();

        revNodes[s] = path;
        nodes[path] = s;
    }
    else
        revNodes[nodes[path]] = path;

    /* Get the dependencies for this expression */
    expression->getDeps(deps);

    /* Insert dependencies into nodes structure */
    std::set<Path>::const_iterator di = deps.begin();
    while (di != deps.end()) {
        if (nodes.find(*di) == nodes.end()) {
            int s = nodes.size();

            nodes[*di] = s;
        }

        edges.push_back(std::make_pair(nodes[path], nodes[*di]));
        ++di;
    }
}

void PropertyExpressionEngine::setValue(const Path & path, const Expression *expr)
{
    Property * prop = docObj->getPropertyByPath(path);

    if (!prop)
        throw Base::Exception("Property not found");


    /* The code below builds a graph for all expressions in the engine, and
     * finds any circular dependencies. It also computes the internal evaluation
     * order, in case properties depends on each other */
    std::map<Path, int> nodes;
    std::map<int, Path> revNodes;
    std::vector<Edge> edges;

    /* Build data structure for graph */
    if (expr)
        buildGraphStructures(path, expr, nodes, revNodes, edges);
    for (std::map<const Path, const Expression*>::const_iterator it = expressions.begin(); it != expressions.end(); ++it) {
        if (it->first != path) // Don't include any old expression for path
            buildGraphStructures(it->first, it->second, nodes, revNodes, edges);
    }

    /* Add edges to graph */
    DiGraph g(edges.size());
    for (std::vector<Edge>::const_iterator i = edges.begin(); i != edges.end(); ++i)
        add_edge(i->first, i->second, g);

    /* Check for cycles */
    bool has_cycle = false;
    cycle_detector vis(has_cycle);
    depth_first_search(g, visitor(vis));

    if (has_cycle)
        throw Base::Exception("Expression creates a cyclic dependency.");

    /* Compute evaluation order for expressions */
    std::vector<int> c;
    topological_sort(g, std::back_inserter(c));

    aboutToSetValue();

    evaluationOrder.clear();
    for (std::vector<int>::iterator i = c.begin(); i != c.end(); ++i) {
        if (revNodes.find(*i) != revNodes.end())
            evaluationOrder.push_back(revNodes[*i]);
    }

    /* Get dependencies for expression */
    if (expr) {
        std::set<Path> deps;
        expr->getDeps(deps);

        /* Add dependencies to property */
        std::set<Path>::const_iterator it = deps.begin();
        while (it != deps.end()) {
            std::string objName = it->getDocumentObjectName();

            if (objName == "")
                objName = docObj->getNameInDocument();

            DocumentObject * target = docObj->getDocument()->getObject(objName.c_str());

            if (target) {
                Property * depprop = target->getPropertyByPath(*it);

                if (depprop)
                    prop->addDependency(*it, depprop, docObj);
            }
            ++it;
        }
    }

    /* Remove old expression first, if any*/
    std::map<const Path, const Expression*>::iterator old = expressions.find(path);
    if (old != expressions.end()) {
        std::set<Path> deps;

        old->second->getDeps(deps);

        /* Remove dependencies from property */
        std::set<Path>::const_iterator it = deps.begin();
        while (it != deps.end()) {
            std::string objName = it->getDocumentObjectName();

            if (objName == "")
                objName = docObj->getNameInDocument();

            DocumentObject * target = docObj->getDocument()->getObject(objName.c_str());

            if (target) {
                Property * depprop = target->getPropertyByPath(*it);

                if (depprop)
                    prop->removeDependency(*it, depprop, docObj);
            }
            ++it;
        }
        delete expressions[path];
        expressions.erase(old);
    }

    if (expr)
        expressions[path] = expr;

    hasSetValue();
}

DocumentObjectExecReturn *App::PropertyExpressionEngine::execute()
{
    std::vector<Path>::const_iterator it = evaluationOrder.begin();

    if (running)
        return DocumentObject::StdReturn;

    running = true;
    while (it != evaluationOrder.end()) {
        Expression * e = expressions[*it]->eval();
        NumberExpression * result;
        Property * prop = docObj->getPropertyByPath(*it);

        result = dynamic_cast<NumberExpression*>(e);
        if (result) {
            App::PropertyContainer* parent = prop->getContainer();

            if (parent->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()))
                docObj->setValue(*it, result);
        }

        delete e;

        ++it;
    }

    running = false;
    return DocumentObject::StdReturn;
}
