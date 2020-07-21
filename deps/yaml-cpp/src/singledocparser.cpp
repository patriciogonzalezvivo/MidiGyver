#include <algorithm>
#include <cstdio>
#include <sstream>

#include "collectionstack.h"  // IWYU pragma: keep
#include "scanner.h"
#include "singledocparser.h"
#include "tag.h"
#include "token.h"
#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/exceptions.h"  // IWYU pragma: keep
#include "yaml-cpp/mark.h"
#include "yaml-cpp/null.h"

namespace YAML {
SingleDocParser::SingleDocParser(Scanner& scanner, const Directives& directives)
    : m_scanner(scanner),
      m_directives(directives),
      m_pCollectionStack(new CollectionStack),
      m_curAnchor(0) {}

SingleDocParser::~SingleDocParser() {}

// HandleDocument
// . Handles the next document
// . Throws a ParserException on error.
void SingleDocParser::HandleDocument(EventHandler& eventHandler) {
  assert(!m_scanner.empty());  // guaranteed that there are tokens
  assert(!m_curAnchor);

  const Token& token = m_scanner.peek_unsafe();
  eventHandler.OnDocumentStart(token.mark);

  // eat doc start
  if (token.type == Token::DOC_START)
    m_scanner.pop();

  // recurse!
  HandleNode(eventHandler);

  eventHandler.OnDocumentEnd();

  // and finally eat any doc ends we see
  while (!m_scanner.empty() && m_scanner.peek_unsafe().type == Token::DOC_END)
    m_scanner.pop_unsafe();
}

Token::TYPE SingleDocParser::HandleNodeOpen(EventHandler& eventHandler) {

  // an empty node *is* a possibility
  if (m_scanner.empty()) {
    eventHandler.OnNull(m_scanner.mark(), NullAnchor);
    return Token::NONE;
  }

  Token& head = m_scanner.peek_unsafe();

  // special case: a value node by itself must be a map, with no header
  if (head.type == Token::VALUE) {
    eventHandler.OnMapStart(head.mark, "?", NullAnchor, EmitterStyle::Default);
    return head.type;
  }

  // special case: an alias node
  if (head.type == Token::ALIAS) {
    eventHandler.OnAlias(head.mark, LookupAnchor(head.mark, head.value));
    m_scanner.pop_unsafe();
    return Token::NONE;
  }

  // save location
  Mark mark = head.mark;

  std::string tag;
  anchor_t anchor = NullAnchor;

  bool hasProps = false;
  bool getProps = true;
  while (getProps) {
    const Token& token = m_scanner.peek_unsafe();
    switch (token.type) {
      case Token::TAG:
        ParseTag(tag);
        hasProps = true;
        break;
      case Token::ANCHOR:
        ParseAnchor(anchor);
        hasProps = true;
        break;
      default:
        getProps = false;
        break;
    }
    if (getProps && m_scanner.empty()) {
      // FIXME Throw? - was unhandled before.
    }
  }

  // head token may be invalidated
  Token& token = hasProps ? m_scanner.peek() : head;

  // add non-specific tags
  if (tag.empty())
    tag = (token.type == Token::NON_PLAIN_SCALAR ? '!' : '?');

  // now split based on what kind of node we should be
  if (token.type == Token::PLAIN_SCALAR) {
    if (!IsNullString(token.value)) {
      eventHandler.OnScalar(mark, tag, anchor, std::move(token.value));
    } else {
      eventHandler.OnNull(mark, anchor);
    }
    m_scanner.pop_unsafe();
    return Token::NONE;

  } else if (token.type == Token::NON_PLAIN_SCALAR) {
    eventHandler.OnScalar(mark, tag, anchor, std::move(token.value));
    m_scanner.pop_unsafe();
    return Token::NONE;

  } else if (token.type == Token::FLOW_MAP_START) {
    eventHandler.OnMapStart(mark, tag, anchor, EmitterStyle::Flow);
    return token.type;

  } else if (token.type == Token::BLOCK_MAP_START) {
    eventHandler.OnMapStart(mark, tag, anchor, EmitterStyle::Block);
    return token.type;

  } else if (token.type == Token::FLOW_SEQ_START) {
    eventHandler.OnSequenceStart(mark, tag, anchor, EmitterStyle::Flow);
    return token.type;

  } else if (token.type == Token::BLOCK_SEQ_START) {
    eventHandler.OnSequenceStart(mark, tag, anchor, EmitterStyle::Block);
    return token.type;

  } else {
    if (token.type == Token::KEY) {
      if (m_pCollectionStack->GetCurCollectionType() == CollectionType::FlowSeq) {
        // compact maps can only go in a flow sequence
        eventHandler.OnMapStart(mark, tag, anchor, EmitterStyle::Flow);
        return token.type;
      }
    }
    if (tag[0] == '?') {
      eventHandler.OnNull(mark, anchor);
    } else {
      eventHandler.OnScalar(mark, tag, anchor, "");
    }
  }
  return Token::NONE;
}

void SingleDocParser::HandleNode(EventHandler& eventHandler) {

  Token::TYPE type = HandleNodeOpen(eventHandler);
  if (type == Token::NONE) {
    return;
  } else {
    switch (type) {
      case Token::FLOW_SEQ_START:
        HandleFlowSequence(eventHandler);
        break;
      case Token::BLOCK_SEQ_START:
        HandleBlockSequence(eventHandler);
        break;
      case Token::FLOW_MAP_START:
        HandleFlowMap(eventHandler);
        break;
      case Token::BLOCK_MAP_START:
        HandleBlockMap(eventHandler);
        break;
      case Token::KEY:
        HandleCompactMap(eventHandler);
        break;
      case Token::VALUE:
        HandleCompactMapWithNoKey(eventHandler);
        break;
      default:
        break;
    }
  }
}

void SingleDocParser::HandleBlockSequence(EventHandler& eventHandler) {
  // eat start token
  m_scanner.pop();
  m_pCollectionStack->PushCollectionType(CollectionType::BlockSeq);

  while (1) {
    if (m_scanner.empty())
      throw ParserException(m_scanner.mark(), ErrorMsg::END_OF_SEQ);

    const Token& token = m_scanner.peek();
    Token::TYPE type = token.type;
    if (type != Token::BLOCK_ENTRY && type != Token::BLOCK_SEQ_END)
      throw ParserException(token.mark, ErrorMsg::END_OF_SEQ);

    m_scanner.pop_unsafe();
    if (type == Token::BLOCK_SEQ_END)
      break;

    // check for null
    if (!m_scanner.empty()) {
      const Token& token = m_scanner.peek_unsafe();
      if (token.type == Token::BLOCK_ENTRY ||
          token.type == Token::BLOCK_SEQ_END) {
        eventHandler.OnNull(token.mark, NullAnchor);
        continue;
      }
    }

    HandleNode(eventHandler);
  }

  m_pCollectionStack->PopCollectionType(CollectionType::BlockSeq);

  eventHandler.OnSequenceEnd();
}

void SingleDocParser::HandleFlowSequence(EventHandler& eventHandler) {
  // eat start token
  m_scanner.pop();
  m_pCollectionStack->PushCollectionType(CollectionType::FlowSeq);

  while (1) {
    if (m_scanner.empty())
      throw ParserException(m_scanner.mark(), ErrorMsg::END_OF_SEQ_FLOW);

    // first check for end
    if (m_scanner.peek_unsafe().type == Token::FLOW_SEQ_END) {
      m_scanner.pop_unsafe();
      break;
    }

    // then read the node
    HandleNode(eventHandler);

    if (m_scanner.empty())
      throw ParserException(m_scanner.mark(), ErrorMsg::END_OF_SEQ_FLOW);

    // now eat the separator (or could be a sequence end, which we ignore - but
    // if it's neither, then it's a bad node)
    const Token& token = m_scanner.peek_unsafe();
    if (token.type == Token::FLOW_ENTRY)
      m_scanner.pop_unsafe();
    else if (token.type != Token::FLOW_SEQ_END)
      throw ParserException(token.mark, ErrorMsg::END_OF_SEQ_FLOW);
  }

  m_pCollectionStack->PopCollectionType(CollectionType::FlowSeq);

  eventHandler.OnSequenceEnd();
}

void SingleDocParser::HandleBlockMap(EventHandler& eventHandler) {
  // eat start token
  m_scanner.pop();
  m_pCollectionStack->PushCollectionType(CollectionType::BlockMap);

  while (1) {
    if (m_scanner.empty())
      throw ParserException(m_scanner.mark(), ErrorMsg::END_OF_MAP);

    const Token& token = m_scanner.peek_unsafe();
    if (token.type != Token::KEY && token.type != Token::VALUE &&
        token.type != Token::BLOCK_MAP_END)
      throw ParserException(token.mark, ErrorMsg::END_OF_MAP);

    if (token.type == Token::BLOCK_MAP_END) {
      m_scanner.pop_unsafe();
      break;
    }

    const Mark mark = token.mark;

    // grab key (if non-null)
    if (token.type == Token::KEY) {
      m_scanner.pop_unsafe();
      HandleNode(eventHandler);
    } else {
      eventHandler.OnNull(token.mark, NullAnchor);
    }

    // now grab value (optional)
    if (!m_scanner.empty() && m_scanner.peek_unsafe().type == Token::VALUE) {
      m_scanner.pop_unsafe();
      HandleNode(eventHandler);
    } else {
      eventHandler.OnNull(mark, NullAnchor);
    }
  }

  m_pCollectionStack->PopCollectionType(CollectionType::BlockMap);
  eventHandler.OnMapEnd();
}

void SingleDocParser::HandleFlowMap(EventHandler& eventHandler) {
  // eat start token
  m_scanner.pop();
  m_pCollectionStack->PushCollectionType(CollectionType::FlowMap);

  while (1) {
    if (m_scanner.empty())
      throw ParserException(m_scanner.mark(), ErrorMsg::END_OF_MAP_FLOW);

    const Token& token = m_scanner.peek_unsafe();
    Mark mark = token.mark;
    // first check for end
    if (token.type == Token::FLOW_MAP_END) {
      m_scanner.pop_unsafe();
      break;
    }

    // grab key (if non-null)
    if (token.type == Token::KEY) {
      m_scanner.pop_unsafe();
      HandleNode(eventHandler);
    } else {
      eventHandler.OnNull(mark, NullAnchor);
    }

    // now grab value (optional)
    if (!m_scanner.empty() && m_scanner.peek_unsafe().type == Token::VALUE) {
      m_scanner.pop_unsafe();
      HandleNode(eventHandler);
    } else {
      eventHandler.OnNull(mark, NullAnchor);
    }

    if (m_scanner.empty())
      throw ParserException(m_scanner.mark(), ErrorMsg::END_OF_MAP_FLOW);

    // now eat the separator (or could be a map end, which we ignore - but if
    // it's neither, then it's a bad node)
    const Token& nextToken = m_scanner.peek_unsafe();
    if (nextToken.type == Token::FLOW_ENTRY)
      m_scanner.pop_unsafe();
    else if (nextToken.type != Token::FLOW_MAP_END)
      throw ParserException(nextToken.mark, ErrorMsg::END_OF_MAP_FLOW);
  }

  m_pCollectionStack->PopCollectionType(CollectionType::FlowMap);
  eventHandler.OnMapEnd();
}

// . Single "key: value" pair in a flow sequence
void SingleDocParser::HandleCompactMap(EventHandler& eventHandler) {
  m_pCollectionStack->PushCollectionType(CollectionType::CompactMap);

  // grab key
  Mark mark = m_scanner.peek().mark;
  m_scanner.pop_unsafe();
  HandleNode(eventHandler);

  // now grab value (optional)
  if (!m_scanner.empty() && m_scanner.peek_unsafe().type == Token::VALUE) {
    m_scanner.pop_unsafe();
    HandleNode(eventHandler);
  } else {
    eventHandler.OnNull(mark, NullAnchor);
  }

  m_pCollectionStack->PopCollectionType(CollectionType::CompactMap);
  eventHandler.OnMapEnd();
}

// . Single ": value" pair in a flow sequence
void SingleDocParser::HandleCompactMapWithNoKey(EventHandler& eventHandler) {
  m_pCollectionStack->PushCollectionType(CollectionType::CompactMap);

  // null key
  eventHandler.OnNull(m_scanner.peek().mark, NullAnchor);

  // grab value
  m_scanner.pop_unsafe();
  HandleNode(eventHandler);

  m_pCollectionStack->PopCollectionType(CollectionType::CompactMap);
  eventHandler.OnMapEnd();
}

// ParseProperties
// . Grabs any tag or anchor tokens and deals with them.
bool SingleDocParser::ParseProperties(std::string& tag, anchor_t& anchor) {
  tag.clear();
  anchor = NullAnchor;
  bool hasProps = false;

  while (!m_scanner.empty()) {

    const Token& token = m_scanner.peek_unsafe();

    switch (token.type) {
      case Token::TAG:
        //ParseTag(tag);
        if (!tag.empty())
          throw ParserException(token.mark, ErrorMsg::MULTIPLE_TAGS);

        tag = Tag(token).Translate(m_directives);
        m_scanner.pop_unsafe();
        hasProps = true;
        break;

      case Token::ANCHOR:
        //ParseAnchor(anchor);
        if (anchor)
          throw ParserException(token.mark, ErrorMsg::MULTIPLE_ANCHORS);

        anchor = RegisterAnchor(token.value);
        m_scanner.pop_unsafe();
        hasProps = true;
        break;

      default:
        return hasProps;
    }
  }
  return hasProps;
}

void SingleDocParser::ParseTag(std::string& tag) {
  const Token& token = m_scanner.peek();
  if (!tag.empty())
    throw ParserException(token.mark, ErrorMsg::MULTIPLE_TAGS);

  Tag tagInfo(token);
  tag = tagInfo.Translate(m_directives);
  m_scanner.pop_unsafe();
}

void SingleDocParser::ParseAnchor(anchor_t& anchor) {
  const Token& token = m_scanner.peek();
  if (anchor)
    throw ParserException(token.mark, ErrorMsg::MULTIPLE_ANCHORS);

  anchor = RegisterAnchor(token.value);
  m_scanner.pop_unsafe();
}

anchor_t SingleDocParser::RegisterAnchor(const std::string& name) {
  if (name.empty())
    return NullAnchor;

  return m_anchors[name] = ++m_curAnchor;
}

anchor_t SingleDocParser::LookupAnchor(const Mark& mark,
                                       const std::string& name) const {
  Anchors::const_iterator it = m_anchors.find(name);
  if (it == m_anchors.end())
    throw ParserException(mark, ErrorMsg::UNKNOWN_ANCHOR);

  return it->second;
}
}
