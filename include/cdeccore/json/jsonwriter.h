#pragma once
#include <cctype>

CDEC_NS_BEGIN
// -------------------------------------------------------------------------- //

class JsonExpressFormater;

// -------------------------------------------------------------------------- //

class CDECCOREEXPORT JsonExpressFormater: public Object
{
	DECLARE_REF_CLASS(JsonExpressFormater)

public:
	stringx	IndentChars;
	stringx	NewLineChars;

public:
	stringx Format(ref<JsonNode> expr);

protected:
	void WriteExpression(stringx name, ref<JsonNode> expr, ref<StringBuilder> sb, int level);
	void WriteValue(stringx name, stringx value, ref<StringBuilder> sb, int level);
	void WriteFloatValue(stringx name, double value, ref<StringBuilder> sb, int level);
	void WriteNewLine(ref<StringBuilder> sb);
	void WriteSubDictionary(stringx name, ref<JsonNode> expr, ref<StringBuilder> sb, int level);
	void WriteSubList(stringx name, ref<JsonNode> expr, ref<StringBuilder> sb, int level);
};

// -------------------------------------------------------------------------- //

class CDECCOREEXPORT JsonWriter: public Object
{
	DECLARE_REF_CLASS(JsonWriter)

public:
	stringx		IndentChars;
	stringx		NewLineChars;

protected:
	ref<JsonNode>			m_root, m_top;
	ref<Stack<JsonNode> >	m_stack;

public:
	JsonWriter() { Reset(); }

	// Re-open root node for appending items, if it is a collection node
	JsonWriter(ref<JsonNode> root, bool fReopen = false);
	
	void Reset();

	void WriteNode(stringx name, ref<JsonNode> node);

	void WriteString(stringx name, stringx value)
	{
		WriteNode(name, JsonNode::NewStringNode(value));
	}
	
	void WriteInt(stringx name, INT64 value)
	{
		WriteNode(name, JsonNode::NewIntNode(value));
	}

	void WriteFloat(stringx name, double value)
	{
		WriteNode(name, JsonNode::NewFloatNode(value));
	}

	void WriteBool(stringx name, bool value)
	{
		WriteNode(name, JsonNode::NewBoolNode(value));
	}

	void WriteNull(stringx name)
	{
		WriteNode(name, JsonNode::NewNullNode());
	}

	void BeginDictionary(stringx name);
	void EndDictionary();

	void BeginList(stringx name);
	void EndList();

	ref<JsonNode>	Complete();

	stringx		GetString();
};

// -------------------------------------------------------------------------- //
namespace json_express
{

	// Note: JE is not a Cdec class
	class JE
	{
	public:
		struct Expr
		{
			stringx			Key;
			ref<JsonNode>	Node;

			Expr(stringx _key, ref<JsonNode> _node): Key(_key), Node(_node)
			{
			}
		};

		struct BExpr
		{
			stringx			Key;
			JsonNodeType	NodeType;

			BExpr(stringx _key, JsonNodeType _type): Key(_key), NodeType(_type)
			{
			}
		};

		struct EExpr
		{
			JsonNodeType	NodeType;

			EExpr(JsonNodeType type)
			{
				NodeType = type;
			}
		};
	
		ref<JsonWriter>		m_wr;

		JE()
		{
			m_wr = gc_new<JsonWriter>();
		}

		JE(ref<JsonNode> node, bool fReopen = false)	// Re-open function
		{
			m_wr = gc_new<JsonWriter>(node, fReopen);
		}

		static JE New()
		{
			return JE();
		}

		static JE Reopen(ref<JsonNode> node)
		{
			return JE(node, true);
		}

		static Expr Pair(stringx key, ref<JsonNode> node)
		{
			return Expr(key, node);
		}

		static Expr Pair(stringx key, stringx value)
		{
			return Expr(key, JsonNode::NewStringNode(value));
		}

		static Expr Pair(stringx key, INT64 value)
		{
			return Expr(key, JsonNode::NewIntNode(value));
		}

		// A "WCHAR*" argument (a wide-string constance) prefer to match "bool" type 
		// than "string / stringx" types in function overloading.
		// So this type (bool) must be declared explicitly.
		// Refer to 
		//     http://www.newsmth.net/nForum/#!article/CPlusPlus/355668
		//     http://connect.microsoft.com/VisualStudio/feedback/details/694684/vc-bool-string-wstring
		static Expr PairBool(stringx key, bool value)
		{
			return Expr(key, JsonNode::NewBoolNode(value));
		}

		static Expr Pair(stringx key, JE sub)
		{
			ref<JsonNode> expr = sub.Complete();
			return Expr(key, expr);
		}

		// See method PairBool
		static Expr Bool(bool value)
		{
			return PairBool(NULL, value);
		}

		static Expr None(stringx key)
		{
			return Pair(key, JsonNode::NewNullNode());
		}

		static Expr None()
		{
			return None(NULL);
		}

		static BExpr Dict(stringx key)
		{
			return BExpr(key, JSN_Dictionary);
		}

		static BExpr Dict()
		{
			return Dict(NULL);
		}

		static BExpr List(stringx key)
		{
			return BExpr(key, JSN_NodeList);
		}

		static BExpr List()
		{
			return List(NULL);
		}

		static EExpr EDict()
		{
			return EExpr(JSN_Dictionary);
		}

		static EExpr EList()
		{
			return EExpr(JSN_NodeList);
		}

		JE& operator +(Expr expr)
		{
			m_wr->WriteNode(expr.Key, expr.Node);
			return *this;
		}

		JE& operator +(BExpr expr)
		{
			switch (expr.NodeType)
			{
			case JSN_Dictionary:
				m_wr->BeginDictionary(expr.Key);
				break;
			case JSN_NodeList:
				m_wr->BeginList(expr.Key);
				break;
			default:
				cdec_throw(JsonException(EC_JSON_NoMatchedCollection, 0));
			}
			return *this;
		}

		JE& operator +(EExpr expr)
		{
			switch (expr.NodeType)
			{
			case JSN_Dictionary:
				m_wr->EndDictionary();
				break;
			case JSN_NodeList:
				m_wr->EndList();
				break;
			default:
				cdec_throw(JsonException(EC_JSON_NoMatchedCollection, 0));
			}
			return *this;
		}

		JE& operator +(stringx value)
		{
			m_wr->WriteString(NULL, value);
			return *this;
		}

		JE& operator +(INT64 value)
		{
			m_wr->WriteInt(NULL, value);
			return *this;
		}

		JE& operator +(JE sub)
		{
			ref<JsonNode> node = sub.Complete();
			m_wr->WriteNode(NULL, node);
			return *this;
		}

		stringx GetString()
		{
			ref<JsonNode> node = Complete();
			ref<JsonExpressFormater> jsf = gc_new<JsonExpressFormater>();
			return jsf->Format(node);
		}

		stringx GetString(ref<JsonExpressFormater> jsf)
		{
			ref<JsonNode> node = Complete();
			return jsf->Format(node);
		}

		ref<JsonNode> Complete()
		{
			return m_wr->Complete();
		}
	};

}

// -------------------------------------------------------------------------- //
CDEC_NS_END
